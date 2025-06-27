#include "metaball/runner.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <Qt>
#include <QtWidgets>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "metaball/camera.hpp"
#include "metaball/image.hpp"

namespace metaball {

Runner::Runner(QWidget* parent) : QWidget(parent) {
  // Initialize window
  setWindowTitle("metaball");
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
  auto image = camera_.make_image(height(), width());
  painter.drawImage(0, 0, image);
}

}  // namespace metaball
