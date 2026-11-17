#pragma once
#include "hrn/section/HrnSectionLayout.h"
#include "hrn/section/HrnSectionShape.h"
namespace hrn {
struct HrnSectionOptions {
  double rotation;
  bool auto_resize_enabled;
  HrnSectionSizeUpdateMode size_update_mode;
  HrnSectionOptions()
      : rotation(0),
        auto_resize_enabled(false),
        size_update_mode(HrnSectionSizeUpdateMode::KeepRatioConstant) {}
};
class HrnSectionData {
 public:
  static std::unique_ptr<HrnSectionData> create(std::unique_ptr<HrnSectionShape> shape,
                                                std::unique_ptr<HrnSectionLayout> layout,
                                                const HrnSectionOptions& options);
  std::unique_ptr<HrnSectionData> clone() const;
  const HrnSectionShape& shape() const { return *shape_; }
  const HrnSectionLayout& layout() const { return *layout_; }
  double rotation() const { return options_.rotation; }
  bool setRotation(double rotation);
  bool autoResizeEnabled() const { return options_.auto_resize_enabled; }
  HrnSectionSizeUpdateMode sizeUpdateMode() const { return options_.size_update_mode; }

 private:
  HrnSectionData(std::unique_ptr<HrnSectionShape> shape, std::unique_ptr<HrnSectionLayout> layout,
                 const HrnSectionOptions& options);
  std::unique_ptr<HrnSectionShape> shape_;
  std::unique_ptr<HrnSectionLayout> layout_;
  HrnSectionOptions options_;
};
}  // namespace hrn
