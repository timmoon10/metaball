#pragma once

#include <QPaintEvent>
#include <QtWidgets>
#include <string>
#include <string_view>

#include "metaball/camera.hpp"
#include "metaball/scene.hpp"

namespace metaball {

class Runner : public QWidget {
 public:
  Runner(QWidget* parent = nullptr);

  void run_command_loop();

  /*! GUI user guide */
  std::string help_message() const;

  /*! Current configuration and statistics */
  std::string info_message() const;

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  Scene scene_;
  Camera camera_;

  void run_command(const std::string_view& command,
                   const std::string_view& params);
};

}  // namespace metaball
