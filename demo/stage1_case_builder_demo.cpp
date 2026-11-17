#include <iostream>

#include "HrnSectionCaseFormatter.h"

int main() {
  hrn::HrnSectionTestCase test_case = hrn::makeFiveLevelMappedCircularCase();
  if (!test_case.succeeded) {
    std::cerr << test_case.error << '\n';
    return 1;
  }
  std::cout << hrn::formatStage1Case(test_case);
}
