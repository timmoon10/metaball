#include <iostream>

#include <QtWidgets>

#include "metaball/runner.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  // Initialize runner
  metaball::Runner runner;
  runner.show();
  std::cout << runner.help_message() << std::endl;

  return app.exec();
}
