#pragma once

#include <QPaintEvent>
#include <QtWidgets>
#include <string>

#include "metaball/camera.hpp"

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
  Camera camera_;
};

}  // namespace metaball
