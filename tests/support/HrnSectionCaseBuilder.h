#pragma once
#include <string>

#include "HrnOwnedSegmentGraph.h"
#include "hrn/section/HrnPositionPointInput.h"
#include "hrn/section/HrnSegmentSideRelation.h"
namespace hrn {
struct HrnCircularSectionCaseOptions {
  double radius;
  double rotation;
  bool auto_resize_enabled;
  HrnSectionSizeUpdateMode size_update_mode;
  double spacing;
  HrnLayoutSolveSettings solve_settings;
  HrnCircularSectionCaseOptions()
      : radius(1),
        rotation(0),
        auto_resize_enabled(false),
        size_update_mode(HrnSectionSizeUpdateMode::KeepRatioConstant),
        spacing(0),
        solve_settings() {}
};
struct HrnSectionTestCase {
  HrnOwnedSegmentGraph graph;
  std::unique_ptr<HrnPositionPointInput> input;
  std::vector<HrnSegmentSideRelation> relations;
  double modeling_tolerance;
  bool succeeded;
  std::string error;
  HrnSectionTestCase() : modeling_tolerance(0), succeeded(false) {}
};
class HrnSectionCaseBuilder {
 public:
  HrnSectionCaseBuilder()
      : has_transform_(false), modeling_tolerance_(0), valid_(true), has_root_{false, false} {}
  HrnSectionCaseBuilder& positionTransform(const Matrix4D& transform);
  HrnSectionCaseBuilder& modelingTolerance(double tolerance);
  HrnSectionCaseBuilder& segment(const std::string& label);
  HrnSectionCaseBuilder& parent(const std::string& child, const std::string& parent);
  HrnSectionCaseBuilder& sideRootCircular(ZeHrnSide side,
                                          const HrnCircularSectionCaseOptions& options);
  HrnSectionCaseBuilder& circularSection(ZeHrnSide side, const std::string& segment_label,
                                         const HrnCircularSectionCaseOptions& options);
  HrnSectionCaseBuilder& relate(const std::string& positive, const std::string& negative,
                                bool same_position);
  HrnSectionTestCase build();

 private:
  struct Entry {
    ZeHrnSide side;
    std::string label;
    HrnCircularSectionCaseOptions options;
  };
  struct RelationEntry {
    std::string positive, negative;
    bool same_position;
  };
  static std::unique_ptr<HrnSectionData> createCircular(
      const HrnCircularSectionCaseOptions& options, double tolerance);
  Matrix4D transform_;
  bool has_transform_;
  double modeling_tolerance_;
  bool valid_;
  HrnOwnedSegmentGraph graph_;
  HrnCircularSectionCaseOptions root_options_[2];
  bool has_root_[2];
  std::vector<Entry> entries_;
  std::vector<RelationEntry> relations_;
};
HrnSectionTestCase makeFiveLevelMappedCircularCase();
}  // namespace hrn
