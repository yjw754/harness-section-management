#pragma once
#include "hrn/section/HrnSectionTypes.h"
#include <memory>
namespace hrn {
class HrnSectionShape {
public:
 virtual ~HrnSectionShape() {}
 virtual ZeHrnSectionShapeType type() const = 0;
 virtual HrnSectionDimensions dimensions() const = 0;
 virtual std::unique_ptr<HrnSectionShape> clone() const = 0;
};
class HrnCircularSectionShape : public HrnSectionShape {
public:
 static std::unique_ptr<HrnCircularSectionShape> create(double radius,double tolerance);
 ZeHrnSectionShapeType type() const override { return ZeHrnSectionShapeType::Circular; }
 HrnSectionDimensions dimensions() const override { return HrnSectionDimensions::circle(radius_); }
 std::unique_ptr<HrnSectionShape> clone() const override;
 double radius() const { return radius_; }
private:
 explicit HrnCircularSectionShape(double radius) : radius_(radius) {}
 double radius_;
};
}
