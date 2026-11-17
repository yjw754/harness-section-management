#pragma once

#include <vector>

namespace hrn_test {
struct FrozenCircleCenter {
  size_t id;
  double x;
  double y;
};
struct FrozenCircularLayoutCase {
  std::vector<double> radii;
  double spacing;
  std::vector<FrozenCircleCenter> centers;
  double parent_radius;
  size_t candidates;
  size_t iterations;
};
inline std::vector<FrozenCircularLayoutCase> circularRuleCases() {
  return {
    {{1}, 0, {{0, 0, 0}}, 1, 1, 1},
    {{1, 1}, 0, {{0, 0, 0}, {1, 2, 0}}, 3, 2, 2},
    {{1, 1, 1}, 0, {{0, 0, 0}, {1, 2, 0}, {2, 1, 1.7320508075688772}}, 3, 20, 3},
    {{1, 1, 1, 1, 1, 1}, 0,
      {{0, 0, 0}, {1, 2, 0}, {2, 1, 1.7320508075688772},
       {3, 1, -1.7320508075688772}, {4, -1, 1.7320508075688772},
       {5, -1, -1.7320508075688772}}, 3, 154, 6},
    {{1, 2, 1}, 0,
      {{1, 0, 0}, {0, 3, 0}, {2, 2.3333333333333335, 1.8856180831641265}}, 4, 20, 3},
    {{1, 2, 1}, 0.5,
      {{1, 0, 0}, {0, 3.5, 0}, {2, 2.6071428571428572, 2.3351244340396464}}, 5, 20, 3},
    {{2, 1.5, 0.75, 1.25, 0.5, 1}, 0,
      {{0, 0, 0}, {1, 3.5, 0}, {3, 2.1785714285714284, 2.411706145162019},
       {5, 2.1428571428571428, -2.0995626366712958},
       {2, 0.20607830878059596, 2.7422676256430787},
       {4, 0.67474748667507756, -2.4072215995262396}}, 5, 146, 6}
  };
}
}  // namespace hrn_test
