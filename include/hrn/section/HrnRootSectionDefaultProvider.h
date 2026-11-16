#pragma once
#include "hrn/section/HrnSectionData.h"
namespace hrn {
struct HrnRootSectionDefaults {
 double radius;
 double modeling_tolerance;
 double spacing;
 HrnLayoutSolveSettings solve_settings;
 bool auto_resize_enabled;
 HrnSectionSizeUpdateMode size_update_mode;
 double rotation;
 HrnRootSectionDefaults() : radius(10),modeling_tolerance(1e-6),spacing(0),solve_settings(),auto_resize_enabled(false),size_update_mode(HrnSectionSizeUpdateMode::KeepRatioConstant),rotation(0) {}
};
class HrnRootSectionDefaultProvider {
public:
 explicit HrnRootSectionDefaultProvider(const HrnRootSectionDefaults& defaults) : defaults_(defaults) {}
 std::unique_ptr<HrnSectionData> create(ZeHrnSide side) const;
private:
 HrnRootSectionDefaults defaults_;
};
}
