#include <QtWidgets>
#include <iostream>

#include "metaball/runner.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  // Initialize runner
  metaball::Runner runner;
  runner.show();
  std::cout << runner.help_message() << std::endl;

  // Start command input
  runner.start_command_input();

  // Start event loop
  return app.exec();
}
