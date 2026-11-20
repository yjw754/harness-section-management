# Stage 2 截面布局调试器

这是仓库内部的圆形截面调试页。页面中的布局由现有 Stage 2 C++ 求解器通过 WebAssembly 计算，不需要为每个案例重新编译，也不需要部署服务。

## 启动

首次配置和构建需要已激活的 Emscripten SDK 和 CMake（验证使用 Emscripten 4.0.20，只有 Embind 包装层使用 C++17）。在仓库根目录运行：

```sh
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release \
  -DHRN_MAX_STAGE=2 -DHRN_BUILD_TESTS=OFF -DHRN_BUILD_INTERACTIVE_DEBUG_TOOL=ON
cmake --build build-wasm --parallel
python3 demo/interactive/serve_debugger.py
```

在浏览器打开 `http://127.0.0.1:8002/`，看完用 `Ctrl+C` 停止本地服务器。如果目录已经构建好，只需要最后一条命令。可用 `--port` 和 `--directory` 指定其他端口或已构建的目录。保持整个生成目录完整，不要直接以 `file://` 打开 HTML。

改动代码后重新运行 `cmake --build build-wasm --target hrn_interactive_solver`，在同一个地址普通刷新即可，不用重启服务器或清缓存。构建按内容给 JS、WASM 和 CSS 加指纹；本地服务器不缓存入口 HTML，只允许缓存带指纹的文件。已打开的旧标签页仍使用它加载时的版本，直到刷新。

## 调试案例

- 「成员截面」分为正、负两棵树：各侧用「+ 顶层截面」新增顶层成员，用节点旁的「+ 子截面」直接新增同侧子节点。选中节点后，在下方直接编辑圆形半径、父级、旋转角、间距和自动调径。每侧可以有 0、1 或多个顶层成员。
- 在「两侧 Root」中分别设置正、负侧的半径、旋转角（°）、间距和自动调径。Stage 2 只有在该侧有多个顶层成员时才用侧 Root 做包容排布；否则虚线 Root 只作参考。开启自动调径后，输入框保留原始半径，图中显示求解后的半径。
- 新增映射默认是「同位置」，约束两端的截面圆心一致；切为「普通对应」则不加这个同位置约束。可以让同一个端点同时参加多条不同映射。
- **旋转优先局部求解。** 在已有成功布局上只改旋转角时，如果局部调整仍无法满足约束，正负侧按角度差绕 Position Point 的共同原点整体旋转，诊断显示「已整体旋转正负两侧」。其他输入角不改写，实际角度见所选截面的求解结果。导入或修改尺寸、层级、映射等参数会重新求解；导出的 JSON 不保存这段布局历史。
- [双侧旋转联动案例](../../docs/verification/same_side.json)：导入后选中成员 `n-main`，把旋转角从 90° 改为 135°。两侧的三对关联子截面从当前布局一起转 45°，`p-main` 的输入角不变；改正侧角度也一样。
- [不对称嵌套旋转案例](../../docs/verification/cases/partial_nested.json)：把正侧 Root 改为 45°、负侧 Root 改为 90°可检查联动和父截面让位；重新导入后把成员 `p-3` 从 0° 改为 180°，可检查整体旋转兜底。更多组合见[旋转验证矩阵](../../docs/verification/rotation-matrix.md)。
- [部分映射的 Root 旋转案例](../../docs/verification/cases/same_side_rot.json)：把任一侧 Root 从 45° 改为 90°，两侧的主截面同步旋转 45°。对侧顶层如果发生碰撞，求解器会保持关联圆心和边界不变，尝试重排没有同位置约束的兄弟子树；没有冲突时保留原位。
- 多条同位置映射通过共享端点形成连通组时，同组的同侧兄弟截面允许同心重叠，并以最大半径作为排布占位。不同组仍满足间距要求，所有子截面仍须被父边界包含。普通对应不会产生这种重叠许可。
- 可导入的重叠案例：[2 对 1](../../docs/verification/cases/N_1.json)、[1 对 2](../../docs/verification/cases/1_N.json)、[不同半径与无关截面](../../docs/verification/cases/N_1_mixed_radii.json)、[多对多传递映射](../../docs/verification/cases/N_N_same_position.json)。原始 2 对 1 案例无需修改层级或半径即可求解。
- [映射后的紧凑重排案例](../../docs/verification/cases/deranged.json)：映射改变固定成员圆心后，其余兄弟截面按半径递减顺序，围绕固定成员重新选择相切位置。该案例中大圆同时与两个映射小圆相切，相对 Root 原点的占用半径从约 18.02 降到 14.40；正负两侧均支持此规则。无映射的子树会整体平移，保留相对位置和旋转。
- [单截面跟随案例](../../docs/verification/cases/stage2-singleton-follows.json)：导入后可切换映射的正侧端点，负侧单截面跟随，正侧三圆保持原排布。删除映射后负侧回到原点。这条规则正负对称，只在一侧有多个顶层截面、另一侧总共只有一个成员且只有一条同位置边时生效。
- 默认在数字或文本编辑停下约 300 ms 后重算，也可关闭自动计算并点击「计算」。求解或校验失败时，树和参数仍展示当前草稿，图中只保留并淡化上次成功的布局。
- JSON 导入和导出用于保存、复用完整案例。此版本只接受 `circular`，且 `modelingTolerance` 必须为内核使用的 `1e-9`。以后新增形状时会在页面上直接编辑矩形长、短边或椭圆长、短半轴，不需要先制作或导入文件。

可以从默认的 `1 × 1` 同位置案例开始，手工检查这些变化：

| 检查项 | 在页面操作 |
| --- | --- |
| 空侧与不同顶层数量 | 删除 `n-main` 得到 `1 × 0`，再添加一个正侧顶层成员得到 `N × 0`；删除两侧成员得到 `0 × 0`。 |
| 一对多／多对一 | 在一侧的主截面下新建一个半径 3 的子截面，再把该子截面与对侧主截面做一条同位置映射。保留默认主截面之间的同位置映射。把嵌套移到另一侧即可反向检查。 |
| 映射导致的同侧重叠 | 导入 `N_1.json`，确认正侧两个半径 8 的顶层截面与负侧截面圆心一致；导入 `1_N.json` 检查反方向。重叠截面可通过树或映射行分别选中。 |
| 两侧嵌套不对称 | 在正侧建 `8 → 3`，负侧建 `8 → 4 → 1`。将正侧半径 3 的子截面与负侧半径 4 的子截面做同位置映射。 |
| 失败后保留布局 | 在嵌套案例先求解成功，再把未开启自动调径的正侧父截面半径从 8 改为 2，观察冲突诊断和淡化的旧布局。 |

浏览器回归测试（另需 Python Playwright 和 Chromium）：

```sh
python3 tests/browser/interactive_debugger_smoke_test.py --directory build-wasm/interactive
python3 tests/browser/interactive_debugger_browser_test.py --directory build-wasm/interactive
```

如需验证构建产物缺失后的恢复，在可丢弃的 Emscripten 构建目录运行：

```sh
cmake -DBUILD_DIR=/path/to/disposable-wasm-build -P tests/acceptance/verify_interactive_wasm_rebuild.cmake
```
