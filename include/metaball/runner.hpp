#pragma once

#include <QPaintEvent>
#include <QTimer>
#include <QtWidgets>
#include <atomic>
#include <mutex>
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
  void paintEvent(QPaintEvent* event) override;

 private:
  Scene scene_;
  Camera camera_;

  std::queue<std::string> command_queue_;
  std::mutex command_queue_mutex_;
  std::thread command_input_loop_;
  std::atomic<bool> command_input_loop_is_active_{false};
  QTimer command_processing_timer_;

  void process_commands();

  void run_command(const std::string_view& command,
                   const std::string_view& params);
};

}  // namespace metaball
