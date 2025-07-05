#include <QtWidgets>
#include <iostream>
#include <thread>

#include "metaball/runner.hpp"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  // Initialize runner
  metaball::Runner runner;
  runner.show();
  std::cout << runner.help_message() << std::endl;

  std::thread t(&metaball::Runner::run_command_loop,
                &runner);  /// TODO Handle properly

  return app.exec();
}
