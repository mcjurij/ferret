#include "hello-time.h"

#include <ctime>
#include <iostream>

void print_localtime() {
  std::time_t result = std::time(0);
  std::cout << std::asctime(std::localtime(&result));
}
