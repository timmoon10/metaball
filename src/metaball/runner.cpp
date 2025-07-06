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
      std::cin >> commands;

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
  auto _ = [&ss](const std::string_view& str) { ss << str << "\n"; };

  // Header
  _("Information");
  _("-----------");

  // Return string
  ss << std::flush;
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
  std::vector<std::string> commands;
  {
    std::lock_guard<std::mutex> guard(command_queue_mutex_);
    for (; !command_queue_.empty(); command_queue_.pop()) {
      commands.push_back(std::move(command_queue_.front()));
    }
  }

  // Parse and run commands
  for (const auto& command : commands) {
    /// TODO Multiple commands
    /// TODO Split command and params
    try {
      run_command(command, "");
    } catch (const std::exception& err) {
      std::cout << util::concat_strings(err.what(), "\n") << std::flush;
    }
  }
}

void Runner::run_command(const std::string_view& command,
                         const std::string_view& params) {
  (void)params;  /// TODO Handle params
  if (command == "info") {
    std::cout << info_message() << std::flush;
  } else if (command == "exit" || command == "quit") {
    stop_command_loop();
    QApplication::quit();
  } else {
    throw std::runtime_error(
        util::concat_strings("Unrecognized command: ", command, "\n"));
  }
}

}  // namespace metaball
