#pragma once

#include <QMouseEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QtWidgets>
#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>

#include "metaball/camera.hpp"
#include "metaball/scene.hpp"

namespace metaball {

class Runner : public QWidget {
 public:
  Runner(QWidget* parent = nullptr);
  ~Runner() override;

  void start_command_input();
  void stop_command_input();

  /*! GUI user guide */
  std::string help_message() const;

  /*! Current configuration and statistics */
  std::string info_message() const;

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  Scene scene_;
  Camera camera_;

  static const size_t timer_interval = 50;  // milliseconds
  QTimer timer_;
  bool display_needs_update_{false};

  std::atomic<bool> command_input_is_active_{false};
  std::thread command_input_loop_;
  std::queue<std::string> command_input_queue_;
  std::mutex command_input_queue_mutex_;

  bool camera_drag_enabled_{false};
  Camera::VectorType camera_drag_orientation_;
  std::array<size_t, 2> mouse_position_{0, 0};
  std::array<size_t, 2> last_step_mouse_position_{0, 0};

  enum class MovementMode { Forward, Backward };
  Camera::ScalarType movement_speed_{1};
  std::unordered_set<MovementMode> movement_active_modes_;

  void timer_step();
  void timer_step_command_input();
  void timer_step_camera_drag();
  void timer_step_movement();

  void run_command(const std::string_view& command,
                   const std::string_view& params);
  void update_mouse_position(const QMouseEvent& event);
};

}  // namespace metaball
