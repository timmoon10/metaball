#pragma once

#include <QPaintEvent>
#include <QtWidgets>
#include <string>

#include "metaball/camera.hpp"
#include "metaball/scene.hpp"

namespace metaball {

class Runner : public QWidget {
 public:
  Runner(QWidget* parent = nullptr);

  /*! GUI user guide */
  std::string help_message() const;

  /*! Current configuration and statistics */
  std::string info_message() const;

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  Scene scene_;
  Camera camera_;
};

}  // namespace metaball
