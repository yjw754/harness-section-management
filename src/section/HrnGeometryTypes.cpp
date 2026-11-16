#include "hrn/section/HrnGeometryTypes.h"
#include <cmath>
namespace hrn {
Matrix4D::Matrix4D() : m{{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}} {}
static double dot(const Matrix4D& a, int i, int j) { double d=0; for(int k=0;k<3;++k) d+=a.m[k][i]*a.m[k][j]; return d; }
bool isRigidTransform(const Matrix4D& a, double t) {
 if(!std::isfinite(t) || t<=0) return false;
 for(int i=0;i<4;++i) for(int j=0;j<4;++j) if(!std::isfinite(a.m[i][j])) return false;
 for(int j=0;j<4;++j) if(std::fabs(a.m[3][j]-(j==3?1.0:0.0))>t) return false;
 for(int i=0;i<3;++i) for(int j=0;j<3;++j) if(std::fabs(dot(a,i,j)-(i==j?1.0:0.0))>t) return false;
 const double d=a.m[0][0]*(a.m[1][1]*a.m[2][2]-a.m[1][2]*a.m[2][1])-a.m[0][1]*(a.m[1][0]*a.m[2][2]-a.m[1][2]*a.m[2][0])+a.m[0][2]*(a.m[1][0]*a.m[2][1]-a.m[1][1]*a.m[2][0]);
 return std::fabs(d-1.0)<=t;
}
Point3D transformPoint(const Matrix4D& a, Point2D p) { return {a.m[0][0]*p.x+a.m[0][1]*p.y+a.m[0][3],a.m[1][0]*p.x+a.m[1][1]*p.y+a.m[1][3],a.m[2][0]*p.x+a.m[2][1]*p.y+a.m[2][3]}; }
Point3D center(const Matrix4D& a) { return {a.m[0][3],a.m[1][3],a.m[2][3]}; }
bool operator==(const Matrix4D& a,const Matrix4D& b) { for(int i=0;i<4;++i) for(int j=0;j<4;++j) if(a.m[i][j]!=b.m[i][j]) return false; return true; }
}
