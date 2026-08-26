## 描述

请简要描述本次 PR 解决的问题或新增的功能，可关联 issue（如 `Closes #123`）。

## 变更类型

请勾选适用的类型：

- [ ] Bug 修复（不破坏兼容性的缺陷修复）
- [ ] 新功能（不破坏兼容性的能力新增）
- [ ] 重构（不改变行为的代码整理）
- [ ] 文档（README / CONTRIBUTING / 注释等）
- [ ] 构建 / CI（CMake、GitHub Actions 等）
- [ ] 其他（请说明）

## 影响范围

涉及哪些层 / 模块 / 页面？（如 `src/app_service`、`VpnRuntimeService`、设置页 QML 等）

- [ ] 应用装配层 `src/app`
- [ ] 应用服务层 `src/app_service`
- [ ] 基础服务层 `src/core`（config / repository / service / vpn_manager / system_tray / util 等）
- [ ] ViewModel / QML（含 QML singleton 注册，`src/viewmodels`）
- [ ] 测试 `tests/`
- [ ] 文档

## 测试验证

- [ ] 已按模块依赖顺序完成构建：`cmake --build build -j`
- [ ] 已运行全量测试：`ctest --test-dir build --output-on-failure`
- [ ] 涉及 QML 改动时已人工验证相关页面
- [ ] 新增/调整了对应模块的单元测试（如适用）

## 检查清单

- [ ] 代码注释使用中文，风格与现有代码一致
- [ ] 新文件已加入所属模块的 `CMakeLists.txt`（新 QML 文件已加入根 `QTET_QML_FILES`）
- [ ] 未违反架构边界约定（详见 `CONTRIBUTING.md`「架构边界约定」）
- [ ] 未引入对底层模块的 UI 反向依赖
- [ ] 无未跟踪的生产源码文件被遗漏（`git status` 检查）

## 截图（如涉及 UI 变更）

（可选）
