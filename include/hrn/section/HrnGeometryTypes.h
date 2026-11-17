#pragma once
#if __cplusplus < 201402L
#error "Hrn section core requires C++14"
#endif
namespace hrn {
struct Point2D {
  double x, y;
};
struct Point3D {
  double x, y, z;
};
struct Vector3D {
  double x, y, z;
};
struct HrnBox2D {
  Point2D min, max;
};
struct Matrix4D {
  double m[4][4];
  Matrix4D();
};
struct HrnRigidTransform2D {
  Point2D origin;
  Point2D x_axis;
  Point2D y_axis;
};
bool isRigidTransform(const Matrix4D& matrix, double tolerance);
Point3D transformPoint(const Matrix4D& matrix, Point2D point);
Point3D center(const Matrix4D& matrix);
bool operator==(const Matrix4D& a, const Matrix4D& b);
}  // namespace hrn
