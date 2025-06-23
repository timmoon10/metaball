#pragma once

#include <string>

#include <QPaintEvent>
#include <QtWidgets>

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

};

}  // namespace metaball
