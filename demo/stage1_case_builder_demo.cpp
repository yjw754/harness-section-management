#include <iostream>

#include "HrnSectionCaseBuilder.h"
using namespace hrn;
int main() {
  HrnCircularSectionCaseOptions root, member;
  root.radius = 12;
  member.radius = 2;
  HrnSectionCaseBuilder b;
  b.positionTransform(Matrix4D())
      .modelingTolerance(1e-6)
      .segment("P1")
      .segment("N1")
      .segment("N2")
      .sideRootCircular(ZeHrnSide::Positive, root)
      .sideRootCircular(ZeHrnSide::Negative, root)
      .circularSection(ZeHrnSide::Positive, "P1", member)
      .circularSection(ZeHrnSide::Negative, "N1", member)
      .circularSection(ZeHrnSide::Negative, "N2", member)
      .relate("P1", "N1", true)
      .relate("P1", "N2", false);
  HrnSectionTestCase c = b.build();
  if (!c.succeeded) {
    std::cerr << c.error << '\n';
    return 1;
  }
  std::cout << "Stage 1 candidate: positive members=" << c.input->sectionCount(ZeHrnSide::Positive)
            << ", negative members=" << c.input->sectionCount(ZeHrnSide::Negative) << '\n';
  std::cout << "Root radii: +"
            << c.input->sideRootSection(ZeHrnSide::Positive).shape().dimensions().primary << " -"
            << c.input->sideRootSection(ZeHrnSide::Negative).shape().dimensions().primary << '\n';
  std::cout << "Topology depths: P1=" << c.graph.depth("P1") << " N1=" << c.graph.depth("N1")
            << " N2=" << c.graph.depth("N2") << '\n';
  std::cout << "One-to-many mappings=" << c.relations.size()
            << ", same-position edges=" << c.relations[0].same_position << '\n';
}
