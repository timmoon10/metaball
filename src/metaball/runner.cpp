#include "metaball/runner.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <QBrush>
#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <Qt>
#include <QtWidgets>

namespace metaball {

namespace {

template <typename T>
QPointF to_qpointf(const T& x, const T& y) {
  return QPointF(static_cast<qreal>(x), static_cast<qreal>(y));
}

}  // namespace

Runner::Runner(QWidget* parent) : QWidget(parent) {
  // Initialize window
  setWindowTitle("metaball");
}

std::string Runner::help_message() const {
  std::ostringstream ss;
  auto _ = [&ss] (const std::string_view& str) { ss << str << "\n"; };
  _("---------------");
  _("|  metaball   |");
  _("---------------");
  ss << std::flush;
  return ss.str();
}

std::string Runner::info_message() const {
  std::ostringstream ss;
  auto _ = [&ss] (const std::string_view& str) { ss << str << "\n"; };

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

  // Draw circle
  int y = height() / 2;
  int x = width() / 2;
  const QBrush brush(QColor(255, 255, 255, 128), Qt::SolidPattern);
  painter.setBrush(brush);
  painter.setPen(Qt::NoPen);
  painter.drawEllipse(to_qpointf(x, y), 40, 40);
}

}  // namespace metaball
