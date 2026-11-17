#include "hrn/section/HrnGeometryTypes.h"
#include "hrn/section/HrnSectionTypes.h"
#include "TestSupport.h"
#include <limits>
using namespace hrn;
int main() {
 Matrix4D identity;
 CHECK(isRigidTransform(identity, 1e-9));
 Matrix4D scale = identity; scale.m[0][0] = 2; CHECK(!isRigidTransform(scale, 1e-9));
 Matrix4D shear = identity; shear.m[0][1] = .2; CHECK(!isRigidTransform(shear, 1e-9));
 Matrix4D mirror = identity; mirror.m[0][0] = -1; CHECK(!isRigidTransform(mirror, 1e-9));
 Matrix4D nan = identity; nan.m[0][3] = std::numeric_limits<double>::quiet_NaN(); CHECK(!isRigidTransform(nan, 1e-9));
 Matrix4D last = identity; last.m[3][0] = .1; CHECK(!isRigidTransform(last, 1e-9));
 CHECK_NEAR(normalizeDegrees(-30), 330, 1e-12);
 CHECK(normalizeDegrees(-1e-15) >= 0 && normalizeDegrees(-1e-15) < 360);
 CHECK(HrnSectionDimensions::circle(2).shape_type == ZeHrnSectionShapeType::Circular);
 CHECK_NEAR(transformPoint(identity, Point2D{2,3}).x, 2, 1e-12);
}
