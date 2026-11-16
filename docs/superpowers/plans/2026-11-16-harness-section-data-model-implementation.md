# Harness Section Management Three-Stage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 以 C++14 严格分三阶段实现可构造测试案例的线束截面模型、圆形多层级规则布局，以及圆形/椭圆形/矩形混合规则布局。

**Architecture:** 第一阶段建立 Position Point、Segment、Section Data 和正负侧 Segment 映射的候选输入模型及测试构造器；第二阶段只实现圆形截面的确定性规则布局；第三阶段在不改变事务和层级接口的前提下扩展椭圆、矩形及混合形状。三个阶段使用硬门禁串行推进，任何阶段未完成全部编译、单测和验收，不得开始下一阶段。

**Tech Stack:** C++14、CMake、CTest、C++ 标准库；不引入第三方测试框架或智能/启发式布局依赖。

**Spec:** `docs/superpowers/specs/2026-11-13-harness-position-point-section-data-design.md`；阶段范围以 2026-11-16 用户确认的三阶段约束为准；圆形规则参考 `docs/references/截面管理.md`。

## Global Constraints

- 编译标准固定为 C++14：`CMAKE_CXX_STANDARD 14`、`CMAKE_CXX_STANDARD_REQUIRED ON`、`CMAKE_CXX_EXTENSIONS OFF`。
- 禁止使用 `std::optional`、`std::variant`、`std::make_unique`、结构化绑定及其他 C++17+ API；使用显式 `bool + out parameter`、带 tag 的值对象和 `std::unique_ptr<T>(new T(...))`。
- `HrnPositionPoint` 直接保存唯一的 4×4 右手正交齐次刚体变换，以及正负两侧各自的 Segment、Location、Section 三个平行容器和常驻 Root Section。
- 每侧始终满足 `segments.size() == locations.size() == sections.size()`；正负两侧的长度和下标空间互不相关。
- Section 不保存 Segment 引用、Section 父子关系或父级相对 `x/y`；层级只从本侧 Segment 容器和 Segment 拓扑派生。
- Section、Shape、Layout 均为独占所有权；复制 Position Point 时保留 Segment 非拥有引用，深拷贝所有 Section、Shape、Layout 和两侧 Root Section。
- 正负 Segment 的普通映射只表达业务对应；只有 `same_position == true` 的显式映射边才产生二维中心相等约束。
- 所有 Layout 均为 Rule Based；本计划不声明、不实现、不注册 `HrnIntelligentLayout` 或智能布局分支。
- Layout 不修改用户旋转角；角度以度为单位并规范化到 `[0°, 360°)`。
- 包围盒或外接圆只能用于候选生成和快速排除；最终成功必须通过真实轮廓包含、相交和间距校验。
- 所有正式修改采用候选状态求解、完整校验、一次交换提交；失败时矩阵、Section 参数、两侧容器和 Location 全部回滚。
- 正式对象的每侧 Root Section 始终非空；由调用方显式提供或由 `HrnRootSectionDefaultProvider` 创建，Root 数量变化不得删除或重置其参数。
- 所有求解诊断必须包含阶段、目标、原因、算法版本、迭代次数；存在几何冲突时还必须包含双方目标、实测值、要求值、见证点和按确定性分级的调整建议。
- Stage 3 的凸几何统一使用确定性支持映射：GJK 求分离距离、EPA 求穿透深度，有界方向区间细分验证包含与边界间距；初始方向、相等值决胜、迭代上限和停止容差均由输入及持久化配置决定。
- 不实现面积统计、UI、持久化、沿路径姿态连续性、三维扫掠/Loft、成员位置锁定或智能布局。

## Review Focus

- 测试构造器缺失矩阵、任一 Root Section、求解配置，或生成重复、空、跨侧错误的 Segment 引用时必须返回构造失败，不能生成半初始化案例（Task 5）。
- 非刚体矩阵以及负侧错误翻转坐标轴必须被拒绝；合法矩阵下二维原点必须映射到公共中心（Task 1、Task 9、Task 10）。
- 半径相同的多个圆必须按本侧 Segment 插入顺序稳定排布，重复运行中心和迭代次数完全一致（Task 8）。
- 圆形多层布局中任一深层父边界不足或显式同位置映射冲突时，整个 Position Point 必须回滚（Task 9、Task 10）。
- 椭圆/旋转矩形的代理候选通过但真实轮廓越界或干涉时，必须由最终几何校验拒绝并继续确定性候选遍历（Task 12、Task 13、Task 15）。

---

## 阶段门禁与 Demo/正式代码边界

```text
Stage 1: 测试输入模型与辅助构造器
  Gate 1 全部通过
        ↓ 才允许开始
Stage 2: 仅圆形、多层级、Rule Based Layout
  Gate 2 全部通过
        ↓ 才允许开始
Stage 3: 圆形 + 椭圆形 + 矩形、Rule Based Layout
  Gate 3 全部通过 → 当前项目范围完成
```

- Demo：仅 `demo/stage1_case_builder_demo.cpp`，用于演示 Stage 1 构造 API；不被正式库或单元测试链接，不作为布局正确性的证据。
- 正式代码：`include/hrn/section/` 和 `src/section/` 中的模型、几何、布局及事务实现。
- 测试辅助代码：`tests/support/` 中拥有 Segment 生命周期并组装候选输入的 Builder；它可以创建尚未布局的 `HrnPositionPointInput`，但不能直接制造一个声称已经合法求解的正式 `HrnPositionPoint`。
- Stage 2/3 只有通过求解器和事务提交后才产生正式 `HrnPositionPoint`；测试 Builder 不得成为绕过正式校验的后门。
- Stage 1 所称“构造 Position Point”明确指构造完整、可校验但尚未布局的 `HrnPositionPointInput`；正式 `HrnPositionPoint` 必须等 Stage 2 求解成功后创建。这避免用占位 Location 破坏“正式对象始终已求解”的设计不变量。

## 执行前产品集成门禁

当前仓库没有 ZW3D 产品源码和既有构建文件。Task 1 开始前创建 `cmake/HrnProductIntegration.cmake` 和 `docs/verification/product-integration-profile.md`：记录产品源码根目录、实际 C++14 编译器、Point/Matrix/Segment 适配类型、`HrnCrossSectLayout.cpp`、`HrnPosPntCrt.cpp`、`HarnessAlg.cpp` 的解析后路径及基线提交号。`HRN_ENABLE_PRODUCT_ADAPTER=ON` 时任一路径不存在必须在 CMake configure 阶段失败；未提供产品仓库时只允许构建 standalone core，不得宣称完成产品集成。

## C++14 类型适配

| 设计文档概念 | C++14 正式接口 |
| --- | --- |
| `std::optional<size_t>` | `bool findSectionIndex(..., size_t& index) const` |
| 可选 Side/数值/见证点 | `bool has_*` 加对应值字段 |
| `std::variant` Dimensions | `HrnSectionDimensions` 的 `shape_type + primary + secondary` 带 tag 值对象 |
| `std::variant` AdjustmentValue | `HrnSectionAdjustmentValue` 的 `value_type` tag 加独立值字段 |
| `std::make_unique<T>` | `std::unique_ptr<T>(new T(...))`，只封装在工厂/clone 内 |
| `std::span` | `const std::vector<T>&` |

`HrnSectionDimensions` 只作为 Shape 接口的瞬时传输值，不作为 Section 的固定字段存储，因此不改变“Section 组合多态 Shape”的设计。

## 文件结构

| 路径 | 职责 | 首次进入阶段 |
| --- | --- | --- |
| `CMakeLists.txt` | C++14、正式库、Demo、CTest、告警和 ASan 选项 | Stage 1 |
| `include/hrn/section/HrnGeometryTypes.h` | Point、Vector、Matrix、二维变换和建模容差 | Stage 1 |
| `include/hrn/section/HrnSectionTypes.h` | C++14 tag 值类型、Side、Shape/Layout 枚举、Options | Stage 1 |
| `include/hrn/section/HrnSegment.h` | 非拥有父子拓扑 | Stage 1 |
| `include/hrn/section/HrnSectionShape.h` | Shape 抽象接口及分阶段启用的 Shape | Stage 1 |
| `include/hrn/section/HrnSectionLayout.h` | 仅 Rule Based Layout 参数 | Stage 1 |
| `include/hrn/section/HrnSectionData.h` | Shape、Layout 和 Options 的独占组合 | Stage 1 |
| `include/hrn/section/HrnRootSectionDefaultProvider.h` | 两侧 Root Section 的统一领域默认创建接口 | Stage 1 |
| `include/hrn/section/HrnPositionPointInput.h` | 尚未求解的两侧候选输入 DTO | Stage 1 |
| `include/hrn/section/HrnPositionPoint.h` | 已求解正式对象及只读/受控写接口 | Stage 1/2 |
| `include/hrn/section/HrnSegmentSideRelation.h` | 正负侧映射边和显式同位置标记 | Stage 1 |
| `tests/support/HrnSectionCaseBuilder.*` | 可复用案例创建 API 与 Segment 生命周期所有者 | Stage 1 |
| `include/hrn/section/HrnSectionGeometryService.h` | 真实轮廓距离、包含、相交 | Stage 2/3 |
| `src/section/HrnConvexGeometryService.cpp` | Stage 3 的 GJK/EPA 与有界方向区间包含校验 | Stage 3 |
| `include/hrn/section/HrnRuleBasedLayoutSolver.h` | 单层 Rule Based Layout 端口 | Stage 2 |
| `include/hrn/section/HrnPositionPointSolver.h` | 多层级、跨侧约束和整点求解 | Stage 2 |
| `include/hrn/section/HrnSectionDiagnostics.h` | 冲突、失败阶段、建议和事务结果 | Stage 2 |
| `src/section/*` | 对应正式实现，每个文件单一职责 | 各阶段 |
| `tests/unit/*_test.cpp` | 组件单测 | 各阶段 |
| `tests/acceptance/stage*_acceptance_test.cpp` | 阶段硬门禁 | 各阶段 |
| `cmake/HrnProductIntegration.cmake` | 产品源码路径解析和适配器构建开关 | Stage 1 |
| `docs/verification/product-integration-profile.md` | 实际产品文件、编译器和基线提交溯源 | Stage 1 |

## 总依赖关系

```text
Task 1 → Task 2 → Task 3 → Task 4 → Task 5 → Task 6 (Gate 1)
                                                    ↓
Task 7 → Task 8 → Task 9 → Task 10 → Task 11 (Gate 2)
                                                    ↓
Task 12 → Task 13 → Task 14 → Task 15 → Task 16 (Gate 3)
```

---

# Stage 1：测试输入模型与辅助创建函数

## Stage 1 验收范围

能够用 C++14 API 构造 Position Point 候选输入、任意 Segment 父子层级、圆形 Section Data、正负侧独立成员以及一对一/一对多/多对一映射边；不执行布局，不声称 Location 已通过几何校验。

### Task 1: 建立 C++14 构建与基础值类型

**Files:**
- Create: `CMakeLists.txt`
- Create: `cmake/HrnProductIntegration.cmake`
- Create: `docs/verification/product-integration-profile.md`
- Create: `include/hrn/section/HrnGeometryTypes.h`
- Create: `src/section/HrnGeometryTypes.cpp`
- Create: `include/hrn/section/HrnSectionTypes.h`
- Create: `src/section/HrnSectionTypes.cpp`
- Create: `tests/TestSupport.h`
- Test: `tests/unit/hrn_cpp14_types_test.cpp`

**Interfaces:**
- Consumes: 无。
- Produces: `Point2D`、`Point3D`、`Vector3D`、`Matrix4D`、`HrnRigidTransform2D`、`HrnBox2D`、`ZeHrnSide`、`ZeHrnSectionShapeType`、`HrnSectionDimensions`；`bool isRigidTransform(...)`、`double normalizeDegrees(...)`。

- [ ] **Step 1: 写失败测试**

```cpp
CHECK(isRigidTransform(identity, 1e-9));
CHECK(!isRigidTransform(scale, 1e-9));
CHECK(!isRigidTransform(shear, 1e-9));
CHECK(!isRigidTransform(mirror, 1e-9));
CHECK(!isRigidTransform(matrixContainingNaN, 1e-9));
CHECK(!isRigidTransform(invalidLastRow, 1e-9));
CHECK_NEAR(normalizeDegrees(-30.0), 330.0, 1e-12);
CHECK(HrnSectionDimensions::circle(2.0).shape_type == ZeHrnSectionShapeType::Circular);
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake -S . -B build -DHRN_BUILD_TESTS=ON -DHRN_BUILD_DEMO=OFF && cmake --build build --target hrn_cpp14_types_test --parallel`

Expected: FAIL，类型和目标尚不存在。

- [ ] **Step 3: 实现 C++14 值类型与构建选项**

添加编译期守卫：当 `__cplusplus < 201402L` 时 `#error`；不得用兼容宏偷偷启用 C++17 类型。`HRN_MAX_STAGE` 默认值为 `1`，后续阶段必须显式提高。实现产品路径 configure 校验，并在集成档案中记录当前仓库只能执行 standalone core 还是已解析到实际产品文件。

- [ ] **Step 4: 运行单测**

Run: `ctest --test-dir build --output-on-failure -R hrn_cpp14_types_test`

Expected: PASS。

- [ ] **Step 5: 提交**

```bash
git add CMakeLists.txt cmake/HrnProductIntegration.cmake docs/verification/product-integration-profile.md include/hrn/section/HrnGeometryTypes.h include/hrn/section/HrnSectionTypes.h src/section/HrnGeometryTypes.cpp src/section/HrnSectionTypes.cpp tests
git commit -m "feat: add c++14 section value types"
```

### Task 2: 实现 Segment 拓扑和测试案例生命周期容器

**Files:**
- Create: `include/hrn/section/HrnSegment.h`
- Create: `src/section/HrnSegment.cpp`
- Create: `tests/support/HrnOwnedSegmentGraph.h`
- Create: `tests/support/HrnOwnedSegmentGraph.cpp`
- Test: `tests/unit/hrn_segment_graph_test.cpp`

**Interfaces:**
- Consumes: Task 1 基础类型。
- Produces: `HrnSegment::parent()`、`children()`；测试接口 `bool HrnOwnedSegmentGraph::tryAdd(const std::string&, HrnSegment*&)`、`setParent(child, parent)`、`find(label)`，由 Graph 独占 Segment 生命周期。

- [ ] **Step 1: 写层级、重复父级和环测试**

```cpp
HrnSegment* root = 0;
HrnSegment* child = 0;
HrnSegment* duplicate = 0;
CHECK(graph.tryAdd("root", root));
CHECK(graph.tryAdd("child", child));
CHECK(graph.setParent(*child, *root));
CHECK(child->parent() == root);
CHECK(!graph.setParent(*root, *child));
CHECK(!graph.tryAdd("root", duplicate));
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --target hrn_segment_graph_test --parallel`

Expected: FAIL，Segment Graph 尚不存在。

- [ ] **Step 3: 实现非拥有拓扑与测试所有者**

生产 `HrnSegment` 不保存测试 label；label 只存在于 `HrnOwnedSegmentGraph` 的查找表中。

- [ ] **Step 4: 运行单测并提交**

Run: `ctest --test-dir build --output-on-failure -R hrn_segment_graph_test`

```bash
git add include/hrn/section/HrnSegment.h src/section/HrnSegment.cpp tests/support tests/unit/hrn_segment_graph_test.cpp
git commit -m "feat: add segment topology test graph"
```

### Task 3: 实现圆形 Section Data 与 Rule Based Layout 参数

**Files:**
- Create: `include/hrn/section/HrnSectionShape.h`
- Create: `src/section/HrnCircularSectionShape.cpp`
- Create: `include/hrn/section/HrnSectionLayout.h`
- Create: `src/section/HrnRuleBasedLayout.cpp`
- Create: `include/hrn/section/HrnSectionData.h`
- Create: `src/section/HrnSectionData.cpp`
- Test: `tests/unit/hrn_section_data_test.cpp`

**Interfaces:**
- Consumes: Task 1 Shape 类型和角度函数。
- Produces: `HrnSectionShape` 抽象接口；Stage 1 只提供 `HrnCircularSectionShape::create(radius, tolerance)`；只提供 `HrnRuleBasedLayout`；`HrnSectionData(shape, layout, HrnSectionOptions)` 和 `clone()`。

- [ ] **Step 1: 写圆形参数、Options 和 clone 测试**

```cpp
CHECK(HrnCircularSectionShape::create(0.0, 1e-6).get() == 0);
CHECK(section.rotation() == 0.0);
section.setRotation(390.0);
CHECK(section.rotation() == 30.0);
CHECK(!invalidLayout.setSpacingTolerance(-0.1));
CHECK(!invalidLayout.setSolveSettings(HrnLayoutSolveSettings(0, 0, 0.0)));
CHECK(section.autoResizeEnabled() == false);
CHECK(section.sizeUpdateMode() == HrnSectionSizeUpdateMode::KeepRatioConstant);
CHECK(section.layout().type() == ZeHrnSectionLayoutType::RuleBased);
std::unique_ptr<HrnSectionData> copy = section.clone();
CHECK(&copy->shape() != &section.shape());
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --target hrn_section_data_test --parallel`

Expected: FAIL，Section Data 尚不存在。

- [ ] **Step 3: 实现圆形 Shape、Rule Based 参数和深拷贝**

Layout 持久化并校验 `spacing_tolerance >= 0`、有效 `algorithm_version`、`max_iterations > 0` 和 `convergence_tolerance > 0`；Section 完整保存 rotation、auto-resize 和 size-update mode；代码树中不得出现 Intelligent Layout 类。

- [ ] **Step 4: 运行单测并提交**

Run: `ctest --test-dir build --output-on-failure -R hrn_section_data_test`

```bash
git add include/hrn/section/HrnSectionShape.h include/hrn/section/HrnSectionLayout.h include/hrn/section/HrnSectionData.h src/section tests/unit/hrn_section_data_test.cpp
git commit -m "feat: add circular section data"
```

### Task 4: 实现 Position Point 候选输入和正式只读对象

**Files:**
- Create: `include/hrn/section/HrnPositionPointInput.h`
- Create: `src/section/HrnPositionPointInput.cpp`
- Create: `include/hrn/section/HrnRootSectionDefaultProvider.h`
- Create: `src/section/HrnRootSectionDefaultProvider.cpp`
- Create: `include/hrn/section/HrnPositionPoint.h`
- Create: `src/section/HrnPositionPoint.cpp`
- Test: `tests/unit/hrn_position_point_model_test.cpp`

**Interfaces:**
- Consumes: Task 1 Matrix、Task 2 Segment、Task 3 Section Data。
- Produces: `HrnRootSectionDefaults` 配置值和 `HrnRootSectionDefaultProvider(defaults).create(ZeHrnSide)`；`HrnPositionPointInput::create(matrix, posRoot, negRoot)`、`append(side, segment, section)` 及 Input 的 Root 查询；正式对象的显式 Root/default-provider 两个 `createEmpty(...)`、`clone()`、`sectionCount()`、`segmentAt()`、`locationAt()`、`sectionAt()`、`sideRootSection()`、`getEffectiveRootSection()`、`bool findSectionIndex(..., size_t& index)`、`validateSectionData(...)`。

- [ ] **Step 1: 写输入一致性和 0/1/N Root 测试**

```cpp
CHECK(input.append(Pos, root, section()));
CHECK(!input.append(Pos, root, section()));
CHECK(HrnPositionPointInput::create(validMatrix, nullRoot(), root()).get() == 0);
CHECK(empty->getEffectiveRootSection(Pos) == 0);
CHECK(singleInput.getEffectiveRootSection(Pos) == &singleInput.sectionAt(Pos, 0));
CHECK(multipleInput.getEffectiveRootSection(Pos) == &multipleInput.sideRootSection(Pos));
CHECK(defaults.create(Pos)->rotation() == 0.0);
CHECK(defaults.create(Pos)->autoResizeEnabled() == false);
CHECK(defaults.create(Pos)->shape().dimensions() == configuredDefaultDimensions);
CHECK(defaults.create(Pos)->layout().solveSettings() == configuredDefaultSolveSettings);
CHECK(&clone->sideRootSection(Pos) != &source.sideRootSection(Pos));
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --target hrn_position_point_model_test --parallel`

Expected: FAIL，Position Point 类型未实现。

- [ ] **Step 3: 实现候选输入与正式对象边界**

Input 可以没有 Location，但必须持有合法矩阵和两个非空、互不共享的 Root Section；正式对象三个容器必须等长，并且只有 solver/transaction friend 能从成功输出构造非空正式对象。Root 数量在 0/1/N 间变化时只改变有效根选择，不销毁或重置常驻 Root Section。

- [ ] **Step 4: 运行单测并提交**

Run: `ctest --test-dir build --output-on-failure -R hrn_position_point_model_test`

```bash
git add include/hrn/section/HrnPositionPointInput.h include/hrn/section/HrnRootSectionDefaultProvider.h include/hrn/section/HrnPositionPoint.h src/section/HrnPositionPointInput.cpp src/section/HrnRootSectionDefaultProvider.cpp src/section/HrnPositionPoint.cpp tests/unit/hrn_position_point_model_test.cpp
git commit -m "feat: add position point input model"
```

### Task 5: 实现两侧 Segment 映射与案例 Builder

**Files:**
- Create: `include/hrn/section/HrnSegmentSideRelation.h`
- Create: `src/section/HrnSegmentSideRelation.cpp`
- Create: `tests/support/HrnSectionCaseBuilder.h`
- Create: `tests/support/HrnSectionCaseBuilder.cpp`
- Create: `demo/stage1_case_builder_demo.cpp`
- Test: `tests/unit/hrn_section_case_builder_test.cpp`

**Interfaces:**
- Consumes: Task 2 Graph、Task 3 Section Data、Task 4 Input。
- Produces: `HrnSegmentSideRelation{positive, negative, same_position}`；`HrnCircularSectionCaseOptions{radius, rotation, auto_resize_enabled, size_update_mode, spacing, solve_settings}`；`HrnSectionCaseBuilder::positionTransform(matrix)`、`modelingTolerance(value)`、`segment(label)`、`parent(child, parent)`、`sideRootCircular(side, options)`、`circularSection(side, segment, options)`、`relate(pos, neg, samePosition)`、`build()`；`HrnSectionTestCase` 独占 Graph、Input、relations 和容差配置。

- [ ] **Step 1: 写案例创建与映射测试**

```cpp
builder.segment("P1").segment("N1").segment("N2");
builder.positionTransform(validMatrix).modelingTolerance(1e-6);
builder.sideRootCircular(Pos, rootOptions).sideRootCircular(Neg, rootOptions);
builder.circularSection(Pos, "P1", circleOptions(3.0, 0.0, 0.2));
builder.circularSection(Neg, "N1", circleOptions(2.0, 0.0, 0.2));
builder.circularSection(Neg, "N2", circleOptions(1.0, 0.0, 0.2));
builder.relate("P1", "N1", true).relate("P1", "N2", false);
CHECK(builder.build().relations.size() == 2);
CHECK(!invalidBuilderWithWrongSideRelation.build().succeeded);
CHECK(!builderMissingNegativeRoot.build().succeeded);
CHECK(!builderWithDuplicateRelation.build().succeeded);
CHECK(!builderWithConflictingRelationFlags.build().succeeded);
```

- [ ] **Step 2: 运行测试确认失败**

Run: `cmake --build build --target hrn_section_case_builder_test --parallel`

Expected: FAIL，Builder 和映射未实现。

- [ ] **Step 3: 实现 Builder 与 Demo**

一对多/多对一用多条边表达；同一无向端点对不得重复或同时保存相反 `same_position` 标记；关系正端必须存在于正侧、负端必须存在于负侧；普通边不自动升级为同位置边。Builder 缺少矩阵、任一 Root、有效容差或有效 Layout 配置时构造失败；Demo 只打印层级、两侧成员、Root 参数和映射摘要。

- [ ] **Step 4: 运行单测和 Demo**

Run: `ctest --test-dir build --output-on-failure -R hrn_section_case_builder_test`

Run: `cmake -S . -B build-demo -DHRN_BUILD_DEMO=ON -DHRN_BUILD_TESTS=OFF && cmake --build build-demo --target stage1_case_builder_demo --config Debug --parallel && ctest --test-dir build-demo -C Debug -R stage1_case_builder_demo --output-on-failure`

Expected: 单测 PASS；Demo 输出一对多映射摘要，且不输出任何布局成功结论。

- [ ] **Step 5: 提交**

```bash
git add include/hrn/section/HrnSegmentSideRelation.h src/section/HrnSegmentSideRelation.cpp tests/support/HrnSectionCaseBuilder.* tests/unit/hrn_section_case_builder_test.cpp demo/stage1_case_builder_demo.cpp
git commit -m "feat: add section test case builder"
```

### Task 6: Stage 1 硬门禁验收

**Files:**
- Create: `tests/acceptance/stage1_acceptance_test.cpp`
- Create: `docs/verification/stage1.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Task 1—5。
- Produces: 5 层拓扑、双侧不同成员、一对一/一对多/多对一、显式/普通映射的可复用基准案例。

- [ ] **Step 1: 写 Stage 1 验收测试**

```cpp
HrnSectionTestCase c = makeFiveLevelMappedCircularCase();
CHECK(c.input.sectionCount(Pos) != c.input.sectionCount(Neg));
CHECK(c.graph.depth("P5") == 5);
CHECK(countSamePositionRelations(c) == 2);
CHECK(allOwnedObjectsHaveDistinctAddresses(c));
CHECK(c.input.positionTransform() == configuredMatrix);
CHECK(c.input.sideRootSection(Pos).autoResizeEnabled() == configuredRootAutoResize);
CHECK(c.input.sectionAt(Pos, 0).layout().solveSettings() == configuredSettings);
```

- [ ] **Step 2: 运行 Stage 1 完整验收**

Run: `cmake -S . -B build-stage1 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=1 && cmake --build build-stage1 --parallel && ctest --test-dir build-stage1 --output-on-failure`

Expected: Stage 1 全部测试 PASS，构建日志显示 C++14 模式。

- [ ] **Step 3: 检查范围守卫**

Run: `rg -n 'HrnIntelligentLayout|std::optional|std::variant|std::make_unique' include src tests`

Expected: 无匹配。

- [ ] **Step 4: 记录并提交 Gate 1**

`docs/verification/stage1.md` 记录编译器、命令、用例数量、Root/矩阵/Layout 配置覆盖和结果；同时引用 `product-integration-profile.md` 的产品路径状态。只有该文件结论为 PASS 才可开始 Task 7；产品路径未解析时必须明确标记为 standalone core PASS，而不是 product integration PASS。

```bash
git add CMakeLists.txt tests/acceptance/stage1_acceptance_test.cpp docs/verification/stage1.md
git commit -m "test: pass stage one input model gate"
```

---

# Stage 2：仅圆形、多层级、Rule Based Layout

## Stage 2 前置门禁

- `docs/verification/stage1.md` 必须为 PASS。
- Task 8 开始前必须能读取现有 `HrnCrossSectLayout.cpp::HrnGetSection`，或取得其相同输入/输出的可执行基线；若二者都不可用，不得凭 `docs/references/截面管理.md` 中的概述猜测历史决胜细节。

## Stage 2 验收范围

只处理圆形 Section；支持间距、旋转基准、固定/自动父半径、多层 Segment、0/1/N Root、显式跨侧同位置约束、结构化失败和整点回滚。只注册 Rule Based Layout。

### Task 7: 实现圆形真实几何服务

**Files:**
- Create: `include/hrn/section/HrnSectionDiagnostics.h`
- Create: `src/section/HrnSectionDiagnostics.cpp`
- Create: `include/hrn/section/HrnSectionGeometryService.h`
- Create: `src/section/HrnCircularGeometryService.cpp`
- Test: `tests/unit/hrn_circular_geometry_test.cpp`
- Test: `tests/unit/hrn_section_diagnostics_test.cpp`

**Interfaces:**
- Consumes: Task 1 Geometry、Task 3 Circular Shape。
- Produces: `HrnSectionFailureReason`、`HrnSectionFailureStage`、冲突/调整/事务结果类型；`signedDistance(circle, circle)`、`containsWithClearance(parentCircle, childCircle, clearance)`、`intersects(circle, circle)`，返回距离及两个见证点。

- [ ] **Step 1: 写相离、相切、重叠、包含和间距测试**

```cpp
CHECK_NEAR(distance(circle(0, 0, 2), circle(5, 0, 1)).signed_distance, 2.0, 1e-9);
CHECK(!contains(parentR5, childAt4R2, 0.0).contained);
CHECK(contains(parentR5, childAt2R2, 1.0).contained);
CHECK(intersects(circle(0, 0, 2), circle(3, 0, 2)));
CHECK(diagnostic.stage == HrnSectionFailureStage::GeometryValidation);
CHECK(diagnostic.algorithm_version == configuredVersion);
CHECK(diagnostic.iterations_used == configuredIterations);
CHECK(conflict.has_measured_value && conflict.has_required_value);
CHECK(conflict.has_lhs_witness && conflict.has_rhs_witness);
CHECK(adjustment.certainty == HrnSectionAdjustmentCertainty::SufficientForCurrentPlacement);
CHECK(noSolutionAdjustment.certainty == HrnSectionAdjustmentCertainty::HeuristicSuggestion);
CHECK(iterationBudgetAdjustment.parameter == HrnSectionParameterKey::MaxIterations);
CHECK(internalError.adjustments.empty());
```

- [ ] **Step 2: 运行失败测试，实现解析圆几何，再运行通过**

Run before/after: `cmake -S . -B build-stage2-dev -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=2 && cmake --build build-stage2-dev --target hrn_circular_geometry_test hrn_section_diagnostics_test --parallel && ctest --test-dir build-stage2-dev -R 'hrn_(circular_geometry|section_diagnostics)_test' --output-on-failure`

Expected: 实现前 FAIL；实现后 PASS。

- [ ] **Step 3: 提交**

```bash
git add include/hrn/section/HrnSectionDiagnostics.h include/hrn/section/HrnSectionGeometryService.h src/section/HrnSectionDiagnostics.cpp src/section/HrnCircularGeometryService.cpp tests/unit/hrn_circular_geometry_test.cpp tests/unit/hrn_section_diagnostics_test.cpp
git commit -m "feat: add circular section geometry"
```

### Task 8: 固化并实现单层圆形规则布局

**Files:**
- Create: `include/hrn/section/HrnRuleBasedLayoutSolver.h`
- Create: `src/section/HrnCircularRuleBasedLayoutSolver.cpp`
- Create: `tests/fixtures/legacy_circular_layout_cases.h`
- Create: `docs/verification/legacy-circular-layout-baseline.md`
- Test: `tests/unit/hrn_circular_rule_layout_test.cpp`
- Reference: `docs/references/截面管理.md:20`

**Interfaces:**
- Consumes: Task 5 Builder 顺序、Task 7 圆几何；只读参考 `HrnCrossSectLayout.cpp::HrnGetSection`。
- Produces: `HrnCircularRuleBasedLayoutSolver::kAlgorithmVersion`；`solve(const HrnCircularLayoutRequest&) -> HrnRuleLayoutResult`，结果含稳定成员中心、父最小半径、候选计数、迭代次数和失败原因。

- [ ] **Step 1: 从历史实现冻结回归向量**

记录 1、2、3、6 个圆，半径相同、半径乱序和带间距七组输入的排序后 ID、二维中心及最小包络半径；浮点值以 `1e-9` 容差断言。基线文档必须写明产品仓库提交号、解析后源码路径、函数签名、编译器和生成日期；从可执行基线采集时同时记录二进制校验值。

- [ ] **Step 2: 写回归测试并确认失败**

Run: `cmake --build build-stage2-dev --target hrn_circular_rule_layout_test --parallel`

Expected: FAIL，solver 尚不存在。

- [ ] **Step 3: 实现历史规则**

按半径降序并以本侧插入顺序稳定决胜；首圆位于父局部原点，第二圆沿父局部 `+X` 外切；后续候选及中心/尾部推进严格匹配冻结向量；搜索有界、无随机。未知 `algorithm_version` 返回 `InvalidInput`；达到 `max_iterations` 返回 `IterationBudgetExceeded` 并报告准确已用次数。

- [ ] **Step 4: 运行单测并提交**

Run: `ctest --test-dir build-stage2-dev -R hrn_circular_rule_layout_test --output-on-failure`

Expected: 七组回归全部 PASS，重复 100 次输出逐值一致。

```bash
git add include/hrn/section/HrnRuleBasedLayoutSolver.h src/section/HrnCircularRuleBasedLayoutSolver.cpp tests/fixtures/legacy_circular_layout_cases.h tests/unit/hrn_circular_rule_layout_test.cpp docs/verification/legacy-circular-layout-baseline.md
git commit -m "feat: add circular rule based layout"
```

### Task 9: 实现圆形多层级与父半径更新

**Files:**
- Create: `include/hrn/section/HrnPositionPointSolver.h`
- Create: `src/section/HrnCircularHierarchySolver.cpp`
- Test: `tests/unit/hrn_circular_hierarchy_solver_test.cpp`

**Interfaces:**
- Consumes: Task 4 Input/Root、Task 8 单层 solver。
- Produces: `HrnPositionPointSolver::solveCircular(input, relations)`；自底向上求有效半径和局部偏移，自顶向下累积旋转/中心并转换三维 Location。

- [ ] **Step 1: 写 0/1/N Root、5 层和自动半径测试**

```cpp
CHECK(solve(zeroRoot).locations(Pos).empty());
CHECK(solve(singleRoot).location(Pos, root) == input.center());
CHECK(solve(multiRoot).sideRootCenter(Pos) == input.center());
CHECK(fiveLevels.absoluteRotation(leaf) == 75.0);
CHECK(fiveLevels.location(leaf) == transformPoint(configuredMatrix, expectedCenter2D));
CHECK(rotatedParent.childCenter2D() == rotate(parentRotation, childLocalOffset));
CHECK(negativeSideUsesSameXYZBasis(result, configuredMatrix));
CHECK_NEAR(autoResizeParent.radius(), requiredRadius, 1e-9);
CHECK(allThreeCircleSizeModesProduceRadius(requiredRadius));
CHECK(fixedSmallParent.reason == HrnSectionFailureReason::FixedBoundaryInsufficient);
CHECK(leafWithAutoResize.shape().dimensions() == leafOriginalDimensions);
CHECK(inactiveZeroRootSideRoot.shape().dimensions() == originalSideRootDimensions);
CHECK(inactiveSingleRootSideRoot.shape().dimensions() == originalSideRootDimensions);
```

- [ ] **Step 2: 运行失败测试，实现两遍层级求解，再运行通过**

Run before/after: `cmake --build build-stage2-dev --target hrn_circular_hierarchy_solver_test --parallel && ctest --test-dir build-stage2-dev -R hrn_circular_hierarchy_solver_test --output-on-failure`

Expected: 实现前 FAIL；实现后 PASS。

- [ ] **Step 3: 提交**

```bash
git add include/hrn/section/HrnPositionPointSolver.h src/section/HrnCircularHierarchySolver.cpp tests/unit/hrn_circular_hierarchy_solver_test.cpp
git commit -m "feat: solve circular section hierarchies"
```

### Task 10: 实现跨侧显式同位置约束和原子事务

**Files:**
- Create: `src/section/HrnCoupledCircularSolver.cpp`
- Create: `src/section/HrnPositionPointTransaction.cpp`
- Modify: `include/hrn/section/HrnPositionPoint.h`
- Test: `tests/unit/hrn_circular_transaction_test.cpp`

**Interfaces:**
- Consumes: Task 5 relations、Task 9 hierarchy solver。
- Produces: 同位置等价类和祖先闭包；`HrnPositionPoint::createSolved(input, relations, solver)`；`validateSectionData()`、`replaceSectionData()`、`replaceSideRootSectionData()`、`replacePositionTransform()`、`appendSectionItem()`、`removeSectionItem()`、`reorderSectionItems(side, stableSegmentOrder)`、`moveSectionItem(sourceSide, targetSide, segment)` 的候选—求解—校验—交换事务。

- [ ] **Step 1: 写普通映射、显式映射、冲突和回滚测试**

```cpp
CHECK(solve(ordinaryRelation).location(Pos, p) != solve(ordinaryRelation).location(Neg, n));
CHECK(solve(samePositionRelation).location(Pos, p) == solve(samePositionRelation).location(Neg, n));
CHECK(buildGroups({p1EqualsN1, p2EqualsN1})[0].members == stableOrder({p1, p2, n1}));
CHECK(conflictingCase.reason == HrnSectionFailureReason::SamePositionConstraintConflict);
CHECK(snapshot(pointAfterFailedEdit) == snapshot(pointBeforeEdit));
CHECK(budgetFailure.reason == HrnSectionFailureReason::IterationBudgetExceeded);
CHECK(nonConvergent.reason == HrnSectionFailureReason::NonConvergent);
CHECK(finalRecheckFailure.reason == HrnSectionFailureReason::GeometryValidationFailed);
CHECK(point.replacePositionTransform(mirrorMatrix).reason == HrnSectionFailureReason::InvalidInput);
CHECK(snapshot(pointAfterInvalidMatrix) == snapshot(pointBeforeInvalidMatrix));
CHECK(replaceInactiveRootResult.committed && inactiveRootParametersPreservedExceptReplacement(point));
CHECK(reorderResult.committed && triplesRemainAligned(point, Pos));
CHECK(point.findSectionIndex(Pos, targetFromOldIndex, resolvedIndex) && point.segmentAt(Pos, resolvedIndex) == &targetFromOldIndex);
CHECK(moveResult.committed && segmentIsOnlyOnTargetSide(point, movedSegment));
CHECK(wrongLocationCount.reason == HrnSectionFailureReason::InternalError);
CHECK(diagnosticsAreOrderedByStageSideDepthAndStableOrder(result.diagnostics));
```

- [ ] **Step 2: 运行失败测试，实现联合组和原子事务，再运行通过**

Run before/after: `cmake --build build-stage2-dev --target hrn_circular_transaction_test --parallel && ctest --test-dir build-stage2-dev -R hrn_circular_transaction_test --output-on-failure`

Expected: 实现前 FAIL；实现后 PASS，全部写入口保持三容器同序，失败原因互不混淆，所有失败保持正式对象逐值不变。

- [ ] **Step 3: 提交**

```bash
git add include/hrn/section/HrnPositionPoint.h src/section/HrnCoupledCircularSolver.cpp src/section/HrnPositionPointTransaction.cpp tests/unit/hrn_circular_transaction_test.cpp
git commit -m "feat: add atomic circular section solving"
```

### Task 11: Stage 2 硬门禁验收

**Files:**
- Create: `tests/acceptance/stage2_acceptance_test.cpp`
- Create: `docs/verification/stage2.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Task 7—10 与 Stage 1 基准案例。
- Produces: 圆形-only 当前可交付基线。

- [ ] **Step 1: 写 Stage 2 验收测试**

覆盖 100 个圆、5 层、双侧不同树、间距、旋转基准、固定/自动父半径、显式同位置边、所有正式写入口、完整结构化诊断、重复求解和失败回滚；每个成功结果再次用 Task 7 独立复检。

- [ ] **Step 2: 运行 Stage 2 完整验收**

Run: `cmake -S . -B build-stage2 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=2 -DHRN_WARNINGS_AS_ERRORS=ON && cmake --build build-stage2 --parallel && ctest --test-dir build-stage2 --output-on-failure`

Expected: Stage 1+2 全部 PASS，圆形成功结果的重叠、越界、间距不足计数均为 0。

- [ ] **Step 3: 运行 ASan 与范围守卫**

Run: `cmake -S . -B build-stage2-asan -DHRN_MAX_STAGE=2 -DHRN_ENABLE_ASAN=ON && cmake --build build-stage2-asan --parallel && ctest --test-dir build-stage2-asan --output-on-failure`

Run: `rg -n 'HrnIntelligentLayout|HrnEllipticalSectionShape|HrnRectangularSectionShape' include src`

Expected: ASan 无错误；Stage 2 生产实现中无智能、椭圆或矩形分支。

- [ ] **Step 4: 记录并提交 Gate 2**

`docs/verification/stage2.md` 必须记录历史回归向量来源、完整命令和 PASS；否则不得开始 Task 12。

同时记录 100 圆/5 层首次求解与单 Section 编辑重算的时间、候选数和迭代数，作为 Stage 3 不得回退的圆形基线。产品集成档案用于确认外部依赖和路径；实际产品适配源码未进入当前仓库前，本计划只能验收 standalone core。

```bash
git add CMakeLists.txt tests/acceptance/stage2_acceptance_test.cpp docs/verification/stage2.md
git commit -m "test: pass stage two circular layout gate"
```

---

# Stage 3：椭圆、矩形与混合形状 Rule Based Layout

## Stage 3 前置门禁

`docs/verification/stage2.md` 必须为 PASS；Stage 2 圆形回归结果在 Stage 3 中不得改变。

## Stage 3 规则布局决策

为复用已验证的圆形规则且避免引入智能搜索，椭圆和矩形先以各自中心外接圆半径生成确定性候选：圆为 `r`，椭圆为长半轴，矩形为半对角线。候选按 Task 8 的稳定成员顺序枚举所有已放置锚点对；每对按 `+Y`、`-Y` 分支顺序生成外切候选，相等值以 `(anchorAOrder, anchorBOrder, branchOrder, x, y)` 字典序决胜。候选若未通过真实轮廓复检则继续下一个，达到 `max_iterations` 返回 `IterationBudgetExceeded`，候选集合遍历完仍无合法项返回 `NoSolutionFound`。

真实凸几何不依赖第三方库：每种 Shape 提供确定性 `support(direction)` 和 `containsPoint(point, tolerance)`；GJK 使用中心差（重合时 `+X`）作为初始方向求分离距离，EPA 从 GJK 单纯形求穿透深度；相等支持点按 Shape 稳定顺序和坐标字典序决胜。父包含和边界间距使用方向角 `[0, 2π)` 的有界区间细分，利用各 Shape 支持函数的解析导数界计算区间上界，直到上/下界差不大于 `modeling_tolerance` 或达到 Layout 的 `max_iterations`。达到预算返回 `IterationBudgetExceeded`，数值循环返回 `NonConvergent`，最终独立复检失败返回 `GeometryValidationFailed`。该方法不承诺最紧凑或完备，且不得把搜索失败误报为数学不可行。

### Task 12: 实现椭圆 Shape 与真实几何

**Files:**
- Modify: `include/hrn/section/HrnSectionShape.h`
- Create: `src/section/HrnEllipticalSectionShape.cpp`
- Modify: `include/hrn/section/HrnSectionGeometryService.h`
- Create: `src/section/HrnEllipticalGeometry.cpp`
- Create: `src/section/HrnConvexGeometryService.cpp`
- Modify: `tests/support/HrnSectionCaseBuilder.h`
- Modify: `tests/support/HrnSectionCaseBuilder.cpp`
- Test: `tests/unit/hrn_elliptical_geometry_test.cpp`

**Interfaces:**
- Consumes: Task 7 几何服务。
- Produces: `HrnEllipticalSectionShape::create(majorRadius, minorRadius, tolerance)`、`support(direction)`、`containsPoint(...)`；旋转椭圆 contour/bounds；确定性 GJK/EPA/区间细分基础服务；Builder 的 `ellipticalSection(side, segment, options)` 和 `sideRootElliptical(side, options)`。

- [ ] **Step 1: 写非法轴、旋转包围盒、包含和见证点测试**

```cpp
CHECK(HrnEllipticalSectionShape::create(2.0, 3.0, 1e-6).get() == 0);
CHECK_NEAR(ellipse90.bounds().halfWidth(), 2.0, 1e-9);
CHECK(!geometry.containsWithClearance(parentCircle, rotatedEllipse, 0.5).contained);
CHECK(distanceResult.has_lhs_witness && distanceResult.has_rhs_witness);
CHECK(repeatGjkResult(100).isBitwiseStable());
CHECK(intervalBounds.upper - intervalBounds.lower <= modelingTolerance);
CHECK(gjkBudgetFailure.reason == HrnSectionFailureReason::IterationBudgetExceeded);
```

- [ ] **Step 2: 运行失败测试，实现椭圆几何，再运行通过并提交**

Run before/after: `cmake -S . -B build-stage3-dev -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=3 && cmake --build build-stage3-dev --target hrn_elliptical_geometry_test --parallel && ctest --test-dir build-stage3-dev -R hrn_elliptical_geometry_test --output-on-failure`

```bash
git add include/hrn/section/HrnSectionShape.h include/hrn/section/HrnSectionGeometryService.h src/section/HrnEllipticalSectionShape.cpp src/section/HrnEllipticalGeometry.cpp src/section/HrnConvexGeometryService.cpp tests/support/HrnSectionCaseBuilder.h tests/support/HrnSectionCaseBuilder.cpp tests/unit/hrn_elliptical_geometry_test.cpp
git commit -m "feat: add elliptical section geometry"
```

### Task 13: 实现矩形 Shape 与真实几何

**Files:**
- Modify: `include/hrn/section/HrnSectionShape.h`
- Create: `src/section/HrnRectangularSectionShape.cpp`
- Create: `src/section/HrnRectangularGeometry.cpp`
- Modify: `tests/support/HrnSectionCaseBuilder.h`
- Modify: `tests/support/HrnSectionCaseBuilder.cpp`
- Test: `tests/unit/hrn_rectangular_geometry_test.cpp`

**Interfaces:**
- Consumes: Task 7/12 几何服务。
- Produces: `HrnRectangularSectionShape::create(longSide, shortSide, tolerance)`、`support(direction)`、`containsPoint(...)`；旋转矩形 contour/bounds；矩形与圆/椭圆/矩形的真实距离、包含和相交；Builder 的 `rectangularSection(side, segment, options)` 和 `sideRootRectangular(side, options)`。

- [ ] **Step 1: 写非法边、旋转角点、相交和包围盒假阳性测试**

```cpp
CHECK(HrnRectangularSectionShape::create(2.0, 3.0, 1e-6).get() == 0);
CHECK(rotatedRectangle.contour().vertices.size() == 4);
CHECK(broadPhaseOverlaps && !exactGeometry.intersects(rectangle, ellipse));
CHECK(boundsInsideParentBounds && !exactGeometry.containsWithClearance(parentEllipse, rectangle, 0.0).contained);
```

- [ ] **Step 2: 运行失败测试，实现矩形几何，再运行通过并提交**

Run before/after: `cmake --build build-stage3-dev --target hrn_rectangular_geometry_test --parallel && ctest --test-dir build-stage3-dev -R hrn_rectangular_geometry_test --output-on-failure`

```bash
git add include/hrn/section/HrnSectionShape.h src/section/HrnRectangularSectionShape.cpp src/section/HrnRectangularGeometry.cpp tests/support/HrnSectionCaseBuilder.h tests/support/HrnSectionCaseBuilder.cpp tests/unit/hrn_rectangular_geometry_test.cpp
git commit -m "feat: add rectangular section geometry"
```

### Task 14: 实现非圆父尺寸更新模式

**Files:**
- Create: `include/hrn/section/HrnParentSizeSolver.h`
- Create: `src/section/HrnParentSizeSolver.cpp`
- Test: `tests/unit/hrn_parent_size_solver_test.cpp`

**Interfaces:**
- Consumes: Task 12/13 真实轮廓。
- Produces: `HrnParentSizeSolver::solve(request)`；支持 `KeepRatioConstant`、`KeepSmallestValue`、`KeepGreatestValue`，返回新 Shape 或 `SizeUpdateConstraintConflict`。

- [ ] **Step 1: 写三种模式、冻结尺寸冲突和叶子不更新测试**

```cpp
CHECK_NEAR(keepRatio.major / keepRatio.minor, originalRatio, 1e-9);
CHECK_NEAR(keepSmallest.minor, originalMinor, 1e-9);
CHECK_NEAR(keepGreatest.major, originalMajor, 1e-9);
CHECK(frozenConflict.reason == HrnSectionFailureReason::SizeUpdateConstraintConflict);
CHECK(leafResult.dimensions == leafOriginalDimensions);
```

- [ ] **Step 2: 运行失败测试，实现确定性尺寸反求，再运行通过**

Run before/after: `cmake --build build-stage3-dev --target hrn_parent_size_solver_test --parallel && ctest --test-dir build-stage3-dev -R hrn_parent_size_solver_test --output-on-failure`

Expected: 实现后全部候选由真实几何服务再次复检；不得交换长短轴或自动旋转。

- [ ] **Step 3: 提交**

```bash
git add include/hrn/section/HrnParentSizeSolver.h src/section/HrnParentSizeSolver.cpp tests/unit/hrn_parent_size_solver_test.cpp
git commit -m "feat: add parent section size modes"
```

### Task 15: 扩展混合形状 Rule Based Layout

**Files:**
- Create: `src/section/HrnMixedShapeRuleBasedLayoutSolver.cpp`
- Modify: `src/section/HrnCircularHierarchySolver.cpp`
- Test: `tests/unit/hrn_mixed_shape_rule_layout_test.cpp`

**Interfaces:**
- Consumes: Task 8 候选顺序、Task 12/13 真实几何、Task 14 尺寸更新。
- Produces: 原 `HrnRuleBasedLayoutSolver::solve(...)` 的多态 Shape 实现；不增加第二种 Layout 类型。

- [ ] **Step 1: 写混合形状、旋转、外接圆假阳性和稳定性测试**

```cpp
CHECK(solve(circleParentWithEllipseAndRectangle).succeeded);
CHECK(allExactConstraintsPass(result));
CHECK(rejectedProxyCandidateCount(result) > 0);
CHECK(candidateTrace(result) == expectedStableAnchorPairAndBranchOrder);
CHECK(repeat(result, 100).allCentersAndIterationsEqual());
CHECK(noCandidate.reason == HrnSectionFailureReason::NoSolutionFound);
CHECK(exhaustedBudget.reason == HrnSectionFailureReason::IterationBudgetExceeded);
CHECK(proxyAcceptedButExactRejected.continuesToNextCandidate);
```

- [ ] **Step 2: 运行失败测试，实现代理候选加真实复检，再运行通过**

Run before/after: `cmake --build build-stage3-dev --target hrn_mixed_shape_rule_layout_test --parallel && ctest --test-dir build-stage3-dev -R hrn_mixed_shape_rule_layout_test --output-on-failure`

Expected: 实现后 PASS；代理候选失败会继续确定性遍历；候选穷尽与预算耗尽原因不同；历史圆形回归向量逐值不变。

- [ ] **Step 3: 提交**

```bash
git add src/section/HrnMixedShapeRuleBasedLayoutSolver.cpp src/section/HrnCircularHierarchySolver.cpp tests/unit/hrn_mixed_shape_rule_layout_test.cpp
git commit -m "feat: extend rule layout to mixed shapes"
```

### Task 16: Stage 3 最终硬门禁验收

**Files:**
- Create: `tests/acceptance/stage3_acceptance_test.cpp`
- Create: `docs/verification/stage3.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Task 12—15 及全部 Stage 1/2 回归。
- Produces: 当前三阶段最终交付基线。

- [ ] **Step 1: 写最终验收矩阵**

覆盖 Builder 创建圆/椭圆/矩形 Root 和成员、三种形状作为父和子、混合 5 层、独立旋转、三种尺寸模式、双侧不同拓扑、显式同位置映射、真实轮廓假阳性、候选继续遍历、全部诊断类型、失败回滚和 100 次确定性重复。

- [ ] **Step 2: 运行 C++14 Debug/Werror 完整验收**

Run: `cmake -S . -B build-final -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=3 -DHRN_WARNINGS_AS_ERRORS=ON && cmake --build build-final --parallel && ctest --test-dir build-final --output-on-failure`

Expected: Stage 1—3 全部测试 PASS；所有成功布局通过独立真实几何复检。

- [ ] **Step 3: 运行 ASan 和禁用能力检查**

Run: `cmake -S . -B build-final-asan -DHRN_MAX_STAGE=3 -DHRN_ENABLE_ASAN=ON && cmake --build build-final-asan --parallel && ctest --test-dir build-final-asan --output-on-failure`

Run: `rg -n 'HrnIntelligentLayout|IntelligentLayout|std::optional|std::variant|std::make_unique' include src tests`

Expected: ASan 无错误；范围守卫无匹配。

- [ ] **Step 4: 记录性能而不扩大范围**

对 100 个成员、5 层混合形状记录首次布局时间和单次编辑重算时间；重新运行 Stage 2 的圆形性能数据集并与 Gate 2 基线比较，任何算法版本未变但结果、候选数或迭代数变化均视为回归。本阶段只记录混合形状基线，不为达成目标引入智能布局或改变规则结果。若目标硬件和产品构建环境已经写入集成档案，同时核对首次布局 `<= 3 s`、单 Section 编辑反馈 `<= 100 ms` 的参考目标；环境未解析时只报告 standalone 基线，不宣称产品性能达标。

- [ ] **Step 5: 记录并提交 Gate 3**

`docs/verification/stage3.md` 记录编译器、全部命令、测试数量、性能基线和已知 `NoSolutionFound` 数据集。

```bash
git add CMakeLists.txt tests/acceptance/stage3_acceptance_test.cpp docs/verification/stage3.md
git commit -m "test: pass final mixed shape layout gate"
```

## 编译与验收命令汇总

以下命令在单配置生成器下直接使用；Visual Studio 等多配置生成器为所有 `cmake --build` 增加 `--config Debug`，并为对应 `ctest` 增加 `-C Debug`。实际生成器和命令必须记录在各阶段 verification 文档中。

### Stage 1

```bash
cmake -S . -B build-stage1 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=1
cmake --build build-stage1 --parallel
ctest --test-dir build-stage1 --output-on-failure
```

### Stage 2（仅 Gate 1 PASS 后）

```bash
cmake -S . -B build-stage2 -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=2 -DHRN_WARNINGS_AS_ERRORS=ON
cmake --build build-stage2 --parallel
ctest --test-dir build-stage2 --output-on-failure
```

### Stage 3（仅 Gate 2 PASS 后）

```bash
cmake -S . -B build-final -DHRN_BUILD_TESTS=ON -DHRN_MAX_STAGE=3 -DHRN_WARNINGS_AS_ERRORS=ON
cmake --build build-final --parallel
ctest --test-dir build-final --output-on-failure
```

## 当前范围完成标准

1. 三个阶段分别具有独立验收文件，且 Gate 1、Gate 2、Gate 3 按顺序为 PASS。
2. 全部正式目标使用 C++14 编译，源码无 C++17 标准库 API。
3. Stage 1 Builder 能复用地创建多层、双侧和多种映射关系案例，但不能绕过正式求解提交非法 Position Point。
4. Stage 2 的圆形布局与历史规则回归向量一致，支持层级、间距、Root、自动半径、同位置约束和回滚。
5. Stage 3 的圆/椭圆/矩形混合布局保持确定性，所有成功结果通过真实轮廓复检。
6. 项目只有 Rule Based Layout；未实现、注册或暗藏 Intelligent Layout。
