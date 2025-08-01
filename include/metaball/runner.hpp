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

#include "metaball/camera.hpp"
#include "metaball/scene.hpp"

namespace metaball {

class Runner : public QWidget {
 public:
  Runner(QWidget* parent = nullptr);

  void start_command_loop();
  void stop_command_loop();

  /*! GUI user guide */
  std::string help_message() const;

  /*! Current configuration and statistics */
  std::string info_message() const;

 protected:
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* event) override;

 private:
  Scene scene_;
  Camera camera_;

  std::queue<std::string> command_queue_;
  std::mutex command_queue_mutex_;
  std::thread command_input_loop_;
  std::atomic<bool> command_input_loop_is_active_{false};
  QTimer command_processing_timer_;

  bool camera_drag_enabled_{false};
  std::array<size_t, 2> camera_drag_pixel_{0, 0};
  Camera::VectorType camera_drag_orientation_;

  void process_commands();

  void run_command(const std::string_view& command,
                   const std::string_view& params);

  void update_camera_drag(const std::optional<bool>& enabled,
                          const std::optional<std::array<size_t, 2>>& pixel,
                          bool update_camera_drag_orientation);
};

}  // namespace metaball
