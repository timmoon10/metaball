#include "metaball/runner.hpp"

#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <Qt>
#include <QtWidgets>
#include <chrono>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "metaball/camera.hpp"
#include "metaball/image.hpp"
#include "util/string.hpp"
#include "util/vector.hpp"

namespace util {

template <size_t N, typename T>
inline std::string to_string_like(const Vector<N, T>& val) {
  return to_string_like(static_cast<Vector<N, T>::ContainerType>(val));
}

}  // namespace util

namespace metaball {

Runner::Runner(QWidget* parent) : QWidget(parent) {
  // Initialize window
  setWindowTitle("metaball");

  // Initialize timer for processing commands
  connect(&command_processing_timer_, &QTimer::timeout, this,
          &Runner::process_commands);
}

void Runner::start_command_loop() {
  // Reset command loop state
  stop_command_loop();

  // Launch thread with command input loop
  command_input_loop_is_active_ = true;
  command_input_loop_ = std::thread([this] {
    while (true) {
      // Wait until command queue is empty
      while (!command_queue_.empty()) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
      }

      // Stop if loop is disabled
      if (!this->command_input_loop_is_active_) {
        return;
      }

      // Read user input
      std::cout << "> ";
      std::string commands;
      std::getline(std::cin, commands);

      // Stop if loop is disabled
      if (!this->command_input_loop_is_active_) {
        return;
      }

      // Push command to queue
      {
        std::lock_guard<std::mutex> guard(command_queue_mutex_);
        command_queue_.push(std::move(commands));
      }
    }
  });

  // Start processing commands
  command_processing_timer_.start(10);  // 10 ms frequency
}

void Runner::stop_command_loop() {
  // Join thread with command input loop
  command_input_loop_is_active_ = false;
  if (command_input_loop_.joinable()) {
    command_input_loop_.join();
  }
  command_input_loop_ = {};

  // Stop processing commands
  command_processing_timer_.stop();

  // Reset command queue
  {
    std::lock_guard<std::mutex> guard(command_queue_mutex_);
    command_queue_ = {};
  }
}

std::string Runner::help_message() const {
  std::ostringstream ss;
  auto _ = [&ss](const std::string_view& str) { ss << str << "\n"; };
  _("---------------");
  _("|  metaball   |");
  _("---------------");
  ss << std::flush;
  return ss.str();
}

std::string Runner::info_message() const {
  std::ostringstream ss;
  auto _ = [&ss]<typename... Ts>(const Ts&... args) {
    ss << util::concat_strings(args...) << "\n";
  };

  // Header
  _("Information");
  _("-----------");

  // Camera properties
  _("Aperture position: ", camera_.aperture_position());
  _("Image offset: ", camera_.image_offset());
  _("Image rotation: ", camera_.image_rotation());
  _("Film speed: ", camera_.film_speed());

  // Return string
  ss << std::endl;
  return ss.str();
}

void Runner::paintEvent(QPaintEvent*) {
  // Initialize painter
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Render image
  auto image = camera_.make_image(scene_, height(), width());
  painter.drawImage(0, 0, image);
}

void Runner::process_commands() {
  // Return immediately if nothing to be done
  if (command_queue_.empty()) {
    return;
  }

  // Read from command queue
  std::vector<std::string> command_queue_copy;
  {
    std::lock_guard<std::mutex> guard(command_queue_mutex_);
    for (; !command_queue_.empty(); command_queue_.pop()) {
      command_queue_copy.push_back(std::move(command_queue_.front()));
    }
  }

  // Parse and run commands
  for (const auto& commands : command_queue_copy) {
    for (const auto& command : util::split(commands, ";")) {
      auto parsed_command = util::split(command, "=", 2);
      UTIL_CHECK(parsed_command.size() >= 1, "error parsing command (", command,
                 ")");
      UTIL_CHECK(parsed_command.size() <= 2, "error parsing command (", command,
                 ")");
      const auto& name = util::strip(parsed_command[0]);
      const auto& params =
          parsed_command.size() > 1 ? util::strip(parsed_command[1]) : "";
      try {
        run_command(name, params);
      } catch (const std::exception& err) {
        std::cout << util::concat_strings(err.what(), "\n") << std::flush;
      }
    }
  }
}

void Runner::run_command(const std::string_view& name,
                         const std::string_view& params) {
  (void)params;  /// TODO Handle params
  if (name == "") {
  } else if (name == "info") {
    std::cout << info_message() << std::flush;
  } else if (name == "exit" || name == "quit") {
    stop_command_loop();
    QApplication::quit();
  } else if (name == "film speed") {
    const auto val = util::from_string<Camera::ScalarType>(std::string(params));
    camera_.set_film_speed(val);
    update();
  } else {
    throw std::runtime_error(
        util::concat_strings("Unrecognized command: ", name, "\n"));
  }
}

}  // namespace metaball
