#include "hrn/section/HrnGeometryTypes.h"
#include "hrn/section/HrnSectionTypes.h"
#include <gtest/gtest.h>
#include <limits>
using namespace hrn;
TEST(HrnCpp14Types, RigidTransformAndAngleContracts) {
 Matrix4D identity;
 ASSERT_TRUE(isRigidTransform(identity, 1e-9));
 Matrix4D scale = identity; scale.m[0][0] = 2; ASSERT_FALSE(isRigidTransform(scale, 1e-9));
 Matrix4D shear = identity; shear.m[0][1] = .2; ASSERT_FALSE(isRigidTransform(shear, 1e-9));
 Matrix4D mirror = identity; mirror.m[0][0] = -1; ASSERT_FALSE(isRigidTransform(mirror, 1e-9));
 Matrix4D nan = identity; nan.m[0][3] = std::numeric_limits<double>::quiet_NaN(); ASSERT_FALSE(isRigidTransform(nan, 1e-9));
 Matrix4D last = identity; last.m[3][0] = .1; ASSERT_FALSE(isRigidTransform(last, 1e-9));
 ASSERT_NEAR(normalizeDegrees(-30), 330, 1e-12);
 ASSERT_GE(normalizeDegrees(-1e-15), 0);
 ASSERT_LT(normalizeDegrees(-1e-15), 360);
 ASSERT_EQ(HrnSectionDimensions::circle(2).shape_type, ZeHrnSectionShapeType::Circular);
 ASSERT_NEAR(transformPoint(identity, Point2D{2,3}).x, 2, 1e-12);
}
