#include "metaball/runner.hpp"

#include <QPaintEvent>
#include <QPainter>
#include <Qt>
#include <QtWidgets>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "metaball/camera.hpp"
#include "metaball/image.hpp"
#include "util/string.hpp"

namespace metaball {

Runner::Runner(QWidget* parent) : QWidget(parent) {
  // Initialize window
  setWindowTitle("metaball");
}

void Runner::run_command_loop() {
  while (true) {
    // Read user input
    std::cout << "> ";
    std::string commands;
    std::cin >> commands;

    // Execute commands
    try {
      /// TODO Multiple commands
      /// TODO Split command and params
      if (commands == "exit" || commands == "quit") {
        break;
      }
      run_command(commands, "");
    } catch (const std::exception& err) {
      std::cout << util::concat_strings(err.what(), "\n") << std::flush;
    }
  }
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
  auto image = camera_.make_image(scene_, height(), width());
  painter.drawImage(0, 0, image);
}

void Runner::run_command(const std::string_view& command,
                         const std::string_view& params) {
  (void)params;  /// TODO Handle params
  if (command == "info") {
    std::cout << info_message() << std::flush;
  } else {
    throw std::runtime_error(
        util::concat_strings("Unrecognized command: ", command, "\n"));
  }
}

}  // namespace metaball
