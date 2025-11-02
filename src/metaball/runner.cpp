#include "metaball/runner.hpp"

#include <QApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QString>
#include <QTimer>
#include <Qt>
#include <QtWidgets>
#include <chrono>
#include <cmath>
#include <iostream>
#include <mutex>
#include <numbers>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "metaball/camera.hpp"
#include "metaball/image.hpp"
#include "metaball/integrator.hpp"
#include "metaball/scene.hpp"
#include "util/string.hpp"
#include "util/vector.hpp"

namespace util {

template <size_t N, typename T>
inline std::string to_string_like(const Vector<N, T>& val) {
  return to_string_like(static_cast<Vector<N, T>::ContainerType>(val));
}

}  // namespace util

namespace metaball {

Runner::Runner(QWidget* parent)
    : QWidget(parent), integrator_{Integrator::make_integrator("stratified sampling")} {
  // Initialize window
  setWindowTitle("metaball");
  setMouseTracking(true);

  // Initialize scene
  scene_.add_element(SceneElement::make_element("polynomial"));

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
  std::string result;
  auto _ = [&result]<typename... Ts>(const Ts&... args) {
    (..., (result += util::to_string_like(args)));
    result += "\n";
  };
  _("---------------");
  _("|  metaball   |");
  _("---------------");
  _();
  return result;
}

std::string Runner::info_message() const {
  std::string result;
  auto _ = [&result]<typename... Ts>(const Ts&... args) {
    (..., (result += util::to_string_like(args)));
    result += "\n";
  };

  // Header
  _("Information");
  _("================");

  // Scene properties
  _();
  _("Scene");
  _("----------------");
  _("Elements:");
  for (size_t i = 0; i < scene_.num_elements(); ++i) {
    _("  ", i, ": ", scene_.get_element(i).describe());
  }
  _("Density threshold: ", scene_.density_threshold());
  _("Density threshold width: ", scene_.density_threshold_width());

  // Integrator properties
  _();
  _("Integrator");
  _("----------------");
  if (integrator_ == nullptr) {
    _("Integrator: none");
  } else {
    _("Integrator: ", integrator_->describe());
  }

  // Camera properties
  _();
  _("Camera");
  _("----------------");
  _("Aperture position: ", camera_.aperture_position());
  _("Aperture orientation: ", camera_.aperture_orientation());
  _("Row orientation: ", camera_.row_orientation());
  _("Column orientation: ", camera_.column_orientation());
  _("Focal length: ", camera_.focal_length());
  _("Film speed: ", camera_.film_speed());

  // Runner properties
  _();
  _("Runner");
  _("----------------");
  _("Timer interval: ", timer_interval_, " ms");
  _("Movement speed: ", movement_speed_);

  // Return string
  _();
  return result;
}

void Runner::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_Escape:
      stop_command_input();
      QApplication::quit();
      break;
    case Qt::Key_W:
      movement_active_modes_.insert(MovementMode::Forward);
      break;
    case Qt::Key_S:
      movement_active_modes_.insert(MovementMode::Backward);
      break;
    case Qt::Key_A:
      movement_active_modes_.insert(MovementMode::Left);
      break;
    case Qt::Key_D:
      movement_active_modes_.insert(MovementMode::Right);
      break;
    case Qt::Key_Q:
      movement_active_modes_.insert(MovementMode::Counterclockwise);
      break;
    case Qt::Key_E:
      movement_active_modes_.insert(MovementMode::Clockwise);
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
    case Qt::Key_A:
      movement_active_modes_.erase(MovementMode::Left);
      break;
    case Qt::Key_D:
      movement_active_modes_.erase(MovementMode::Right);
      break;
    case Qt::Key_Q:
      movement_active_modes_.erase(MovementMode::Counterclockwise);
      break;
    case Qt::Key_E:
      movement_active_modes_.erase(MovementMode::Clockwise);
      break;
  }
}

void Runner::mousePressEvent(QMouseEvent* event) {
  update_mouse_position(*event);
  if (event->button() == Qt::LeftButton) {
    if (!camera_drag_enabled_) {
      camera_drag_enabled_ = true;
      camera_drag_orientation_ = std::nullopt;
    }
  }
}

void Runner::mouseReleaseEvent(QMouseEvent* event) {
  update_mouse_position(*event);
  if (event->button() == Qt::LeftButton) {
    camera_drag_enabled_ = false;
    camera_drag_orientation_ = std::nullopt;
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
  UTIL_CHECK(integrator_ != nullptr, "Integrator has not been initialized");
  auto image = camera_.make_image(scene_, *integrator_, height(), width());
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
      const auto command = util::split(unparsed_command, "=", 2);
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
  if (camera_drag_orientation_) {
    camera_.set_pixel_orientation(mouse_position_[0], mouse_position_[1],
                                  height(), width(), *camera_drag_orientation_);
    display_needs_update_ = true;
  } else {
    camera_drag_orientation_ = camera_.pixel_orientation(
        mouse_position_[0], mouse_position_[1], height(), width());
  }
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
  if (modes.contains(MovementMode::Left) &&
      modes.contains(MovementMode::Right)) {
    modes.erase(MovementMode::Left);
    modes.erase(MovementMode::Right);
  }
  if (modes.contains(MovementMode::Clockwise) &&
      modes.contains(MovementMode::Counterclockwise)) {
    modes.erase(MovementMode::Clockwise);
    modes.erase(MovementMode::Counterclockwise);
  }
  if (modes.empty()) {
    return;
  }

  // Camera position and orientation
  auto position = camera_.aperture_position();
  auto forward_orientation = camera_.aperture_orientation();
  auto right_orientation = camera_.row_orientation();
  auto down_orientation = camera_.column_orientation();

  // Changing orientation or image configuration will invalidate
  // camera drag state
  bool camera_drag_is_invalidated = false;

  // Translations
  const auto movement_distance =
      static_cast<Camera::ScalarType>(movement_speed_ * step_interval);
  if (modes.contains(MovementMode::Forward)) {
    position += forward_orientation * movement_distance;
  }
  if (modes.contains(MovementMode::Backward)) {
    position -= forward_orientation * movement_distance;
  }
  if (modes.contains(MovementMode::Left)) {
    position -= right_orientation * movement_distance;
  }
  if (modes.contains(MovementMode::Right)) {
    position += right_orientation * movement_distance;
  }

  // Rotations
  if (modes.contains(MovementMode::Clockwise) ||
      modes.contains(MovementMode::Counterclockwise)) {
    auto rotation = movement_distance * std::numbers::pi;
    if (modes.contains(MovementMode::Clockwise)) {
      rotation = -rotation;
    }
    const auto sin = std::sin(rotation);
    const auto cos = std::cos(rotation);
    const auto tmp_down_orientation =
        cos * down_orientation + sin * right_orientation;
    right_orientation = -sin * down_orientation + cos * right_orientation;
    down_orientation = tmp_down_orientation;
    camera_drag_is_invalidated = true;
  }

  // Update camera
  camera_.set_aperture_position(position);
  camera_.set_orientation(forward_orientation, right_orientation,
                          down_orientation);

  // Update camera drag state if needed
  if (camera_drag_enabled_ && camera_drag_is_invalidated) {
    camera_drag_orientation_ = camera_.pixel_orientation(
        mouse_position_[0], mouse_position_[1], height(), width());
  }

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

  // Export commands
  if (name == "save") {
    UTIL_CHECK(integrator_ != nullptr, "Integrator has not been initialized");
    auto image = camera_.make_image(scene_, *integrator_, height(), width());
    const std::string file =
        params.empty() ? "metaball.png" : std::string(params);
    static_cast<QImage>(image).save(QString(file.data()));
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
  if (name == "add scene") {
    scene_.add_element(SceneElement::make_element(params));
    return;
  }
  if (name == "remove scene") {
    size_t idx = 0;
    if (params.empty()) {
      if (scene_.num_elements() > 0) {
        idx = scene_.num_elements() - 1;
      }
    } else {
      idx = util::from_string<size_t>(params);
    }
    scene_.remove_element(idx);
    return;
  }
  if (name == "delete scene") {
    if (params.empty()) {
      if (scene_.num_elements() > 0) {
        scene_.remove_element(scene_.num_elements() - 1);
      }
    } else {
      scene_.remove_element(util::from_string<size_t>(params));
    }
    return;
  }
  if (name == "density threshold") {
    scene_.set_density_threshold(util::from_string<ScalarType>(params));
    return;
  }
  if (name == "density threshold width") {
    scene_.set_density_threshold_width(util::from_string<ScalarType>(params));
    return;
  }
  if (name == "set integrator") {
    integrator_ = Integrator::make_integrator(params);
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
