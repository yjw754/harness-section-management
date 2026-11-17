#include <gtest/gtest.h>

#include "HrnSectionCaseBuilder.h"
#include "HrnSectionCaseFormatter.h"

using namespace hrn;

TEST(HrnSectionCaseFormatter, ShowsBothTreesAndEveryMapping) {
  HrnSectionTestCase c = makeFiveLevelMappedCircularCase();
  ASSERT_TRUE(c.succeeded);
  const std::string expected =
      "Stage 1 候选输入（尚未布局）\n"
      "中心：(10, 20, 30)\n"
      "正侧（6 个 Segment；2 个根）\n"
      "  侧级 Root Section：圆形 r=20；自动尺寸=开；参与多根排布\n"
      "  ├─ P1：圆形 r=2，旋转=0°\n"
      "  │  └─ P2：圆形 r=2，旋转=0°\n"
      "  │     └─ P3：圆形 r=2，旋转=0°\n"
      "  │        └─ P4：圆形 r=2，旋转=0°\n"
      "  │           └─ P5：圆形 r=2，旋转=0°\n"
      "  └─ P6：圆形 r=2，旋转=0°\n"
      "负侧（3 个 Segment；3 个根）\n"
      "  侧级 Root Section：圆形 r=20；自动尺寸=开；参与多根排布\n"
      "  ├─ N1：圆形 r=2，旋转=0°\n"
      "  ├─ N2：圆形 r=2，旋转=0°\n"
      "  └─ N3：圆形 r=2，旋转=0°\n"
      "映射关系（4 条；同位置 2 条）\n"
      "  正:P1 <-> 负:N1 [同位置]\n"
      "  正:P1 <-> 负:N2 [普通对应]\n"
      "  正:P2 <-> 负:N2 [普通对应]\n"
      "  正:P3 <-> 负:N3 [同位置]\n";
  EXPECT_EQ(formatStage1Case(c), expected);
}
