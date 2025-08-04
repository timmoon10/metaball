#include "metaball/runner.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <Qt>
#include <QtWidgets>
#include <chrono>
#include <iostream>
#include <mutex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

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
  setMouseTracking(true);

  // Initialize timer
  connect(&timer_, &QTimer::timeout, this, &Runner::timer_step);
  timer_.start(timer_interval_);
  last_step_time_ = std::chrono::high_resolution_clock::now();
}

Runner::~Runner() { stop_command_input(); }

void Runner::start_command_input() {
  // Reset command loop state
  stop_command_input();

  // Launch thread with command input loop
  command_input_is_active_ = true;
  command_input_loop_ = std::thread([this] {
    while (true) {
      // Wait until command queue is empty
      while (!command_input_queue_.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timer_interval_));
      }

      // Stop if loop is disabled
      if (!this->command_input_is_active_) {
        return;
      }

      // Read user input
      std::cout << "> ";
      std::string commands;
      std::getline(std::cin, commands);

      // Stop if loop is disabled
      if (!this->command_input_is_active_) {
        return;
      }

      // Push command to queue
      {
        std::lock_guard<std::mutex> guard(command_input_queue_mutex_);
        command_input_queue_.push(std::move(commands));
      }
    }
  });
}

void Runner::stop_command_input() {
  // Join thread with command input loop
  command_input_is_active_ = false;
  if (command_input_loop_.joinable()) {
    command_input_loop_.join();
  }
  command_input_loop_ = {};

  // Reset command queue
  {
    std::lock_guard<std::mutex> guard(command_input_queue_mutex_);
    command_input_queue_ = {};
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
  _("Aperture orientation: ", camera_.aperture_orientation());
  _("Row orientation: ", camera_.row_orientation());
  _("Column orientation: ", camera_.column_orientation());
  _("Focal length: ", camera_.focal_length());
  _("Film speed: ", camera_.film_speed());

  // Runner properties
  _("Timer interval: ", timer_interval_, " ms");
  _("Movement speed: ", movement_speed_);

  // Return string
  ss << std::endl;
  return ss.str();
}

void Runner::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_W:
      movement_active_modes_.insert(MovementMode::Forward);
      break;
    case Qt::Key_S:
      movement_active_modes_.insert(MovementMode::Backward);
      break;
  }
}

void Runner::keyReleaseEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_W:
      movement_active_modes_.erase(MovementMode::Forward);
      break;
    case Qt::Key_S:
      movement_active_modes_.erase(MovementMode::Backward);
      break;
  }
}

void Runner::mousePressEvent(QMouseEvent* event) {
  update_mouse_position(*event);
  if (event->button() == Qt::LeftButton) {
    if (!camera_drag_enabled_) {
      camera_drag_enabled_ = true;
      camera_drag_orientation_ = camera_.pixel_orientation(
          mouse_position_[0], mouse_position_[1], height(), width());
      last_step_mouse_position_ = mouse_position_;
    }
  }
}

void Runner::mouseReleaseEvent(QMouseEvent* event) {
  update_mouse_position(*event);
  if (event->button() == Qt::LeftButton) {
    camera_drag_enabled_ = false;
  }
}

void Runner::mouseMoveEvent(QMouseEvent* event) {
  update_mouse_position(*event);
}

void Runner::paintEvent(QPaintEvent*) {
  // Initialize painter
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Render image
  auto image = camera_.make_image(scene_, height(), width());
  painter.drawImage(0, 0, image);
}

void Runner::timer_step() {
  // Compute time since last step
  double step_interval =
      static_cast<double>(timer_interval_) / 1000;  // seconds
  {
    const auto current_step_time = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> measured_duration =
        current_step_time - last_step_time_;
    step_interval = std::max(step_interval, measured_duration.count());
    last_step_time_ = current_step_time;
  }

  // Timer step stages
  timer_step_command_input();
  timer_step_camera_drag();
  timer_step_movement(step_interval);

  // Update display if needed
  if (display_needs_update_) {
    update();
    display_needs_update_ = false;
  }
}

void Runner::timer_step_command_input() {
  // Return immediately if nothing to be done
  if (command_input_queue_.empty()) {
    return;
  }

  // Read from command queue
  std::vector<std::string> input_lines;
  {
    std::lock_guard<std::mutex> guard(command_input_queue_mutex_);
    for (; !command_input_queue_.empty(); command_input_queue_.pop()) {
      input_lines.push_back(std::move(command_input_queue_.front()));
    }
  }

  // Parse and run commands
  for (const auto& input_line : input_lines) {
    for (const auto& unparsed_command : util::split(input_line, ";")) {
      auto command = util::split(unparsed_command, "=", 2);
      UTIL_CHECK(command.size() >= 1, "error parsing command (",
                 unparsed_command, ")");
      UTIL_CHECK(command.size() <= 2, "error parsing command (",
                 unparsed_command, ")");
      const auto& name = util::strip(command[0]);
      const auto& params = command.size() > 1 ? util::strip(command[1]) : "";
      try {
        run_command(name, params);
      } catch (const std::exception& err) {
        std::cout << util::concat_strings(err.what(), "\n") << std::flush;
      }
    }
  }

  // Update display
  display_needs_update_ = true;
}

void Runner::timer_step_camera_drag() {
  // Return immediately if update is not needed
  if (!camera_drag_enabled_) {
    return;
  }
  if (mouse_position_ == last_step_mouse_position_) {
    return;
  }

  // Update cached mouse position
  last_step_mouse_position_ = mouse_position_;

  // Update camera orientation
  camera_.set_pixel_orientation(mouse_position_[0], mouse_position_[1],
                                height(), width(), camera_drag_orientation_);
  display_needs_update_ = true;
}

void Runner::timer_step_movement(double step_interval) {
  // Return immediately if there is no movement
  if (movement_active_modes_.empty()) {
    return;
  }

  // Resolve conflicting movement modes
  auto modes = movement_active_modes_;
  if (modes.contains(MovementMode::Forward) &&
      modes.contains(MovementMode::Backward)) {
    modes.erase(MovementMode::Forward);
    modes.erase(MovementMode::Backward);
  }
  if (modes.empty()) {
    return;
  }

  // Camera position and orientation
  auto position = camera_.aperture_position();
  auto forward_orientation = camera_.aperture_orientation();

  // Compute movement
  const auto movement_distance =
      static_cast<Camera::ScalarType>(movement_speed_ * step_interval);
  if (modes.contains(MovementMode::Forward)) {
    position += forward_orientation * movement_distance;
  }
  if (modes.contains(MovementMode::Backward)) {
    position -= forward_orientation * movement_distance;
  }

  // Update camera
  camera_.set_aperture_position(position);
  camera_.set_aperture_orientation(forward_orientation);

  // Update display
  display_needs_update_ = true;
}

void Runner::run_command(const std::string_view& name,
                         const std::string_view& params) {
  using ScalarType = Scene::ScalarType;

  // Basic commands
  if (name == "") {
    return;
  }
  if (name == "info") {
    std::cout << info_message() << std::flush;
    return;
  }
  if (name == "exit" || name == "quit") {
    stop_command_input();
    QApplication::quit();
    return;
  }

  // Camera commands
  if (name == "reset camera") {
    camera_ = {};
    return;
  }
  if (name == "focal length") {
    camera_.set_focal_length(util::from_string<ScalarType>(params));
    return;
  }
  if (name == "film speed") {
    camera_.set_film_speed(util::from_string<ScalarType>(params));
    return;
  }
  if (Camera::is_adjust_shot_type(name)) {
    camera_.adjust_shot(name, util::from_string<ScalarType>(params));
    return;
  }

  // Scene commands
  if (name == "reset scene") {
    scene_ = {};
    return;
  }

  // Runner commands
  if (name == "movement speed") {
    auto val = util::from_string<Camera::ScalarType>(params);
    UTIL_CHECK(val > 0, "Invalid movement speed (", val, ")");
    movement_speed_ = val;
    return;
  }

  // Throw exception if command is not supported
  throw std::runtime_error(
      util::concat_strings("Unrecognized command: ", name, "\n"));
}

void Runner::update_mouse_position(const QMouseEvent& event) {
  const auto& position = event.position();
  const auto& x = position.x();
  const auto& y = position.y();
  if (0 <= x && x < width() && 0 <= y && y < height()) {
    mouse_position_ = {static_cast<size_t>(y), static_cast<size_t>(x)};
  }
}

}  // namespace metaball
