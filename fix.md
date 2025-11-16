# README 与实现差异汇总

## 1. 示例程序缺失
- 说明：README.md:179-201 明确声称 `examples/` 目录下提供 calculator_server、async_client 等 7 个完整示例，并附有运行指南。
- 现状：仓库的 `examples/` 实际只有空目录 `calculator/` 与 `echo/`，没有任意源文件或 README 所提到的示例实现，导致用户无法按照文档体验示例。
- 影响：示例构建与运行指令全部失效，使用者无法验证库的实际用法，README 中的承诺属实质性失真。

## 2. “编译为库”模式不可用
- 说明：README.md:122-129 提供了关闭 header-only（`cmake -DJSONRPC_HEADER_ONLY=OFF ..`）后编译静态/动态库的指南。
- 现状：顶层 `CMakeLists.txt:35-48` 在该模式下执行 `add_subdirectory(src)`，但仓库缺少 `src/CMakeLists.txt` 或其它构建脚本，仅包含若干 `.cpp` 文件。这会使 CMake 配置阶段立即报错，无法产出目标库。
- 影响：README 承诺的“Header-only 或编译库两种模式”在当前仓库中只有前者可行，文档与实现严重不符。

## 3. Boost 依赖声明不一致
- 说明：README.md:44-47 要求 Boost 1.83+，并列出 JSON、Beast、Asio、System 组件。
- 现状：`CMakeLists.txt:18-19` 仅查找 `Boost 1.66 COMPONENTS system`，既没有约束到 1.83，也未引入 JSON/Beast/Asio，导致 README 中的前置条件无法通过构建脚本保证。
- 影响：依赖版本和组件可能不足以支撑 README 描述的特性，使用者按文档准备环境后仍可能遭遇缺库问题。

## 4. MethodWrapper 模板重复定义
- 现状：`include/jsonrpc/detail/method_wrapper.hpp:171-205` 与同一文件的 `223-272` 行分别定义了两个不同的 `MethodWrapperImpl` 模板，二者签名完全相同。
- 影响：任何翻译单元包含该头文件都会触发“redefinition of ‘template<class Func> class MethodWrapperImpl’”编译错误，库无法编译，更不可能达到 README 的可用状态。

## 5. 测试用例缺失
- 说明：README.md:131-137 指导运行 `./tests/jsonrpc_tests`，`tests/CMakeLists.txt:12-19` 也将 `test_protocol.cpp`、`test_client.cpp` 等 5 个源文件加入编译。
- 现状：`tests/` 目录实际仅包含 `test_types.cpp`，其余文件全部缺失，导致 `cmake --build` 阶段无法找到源文件（No such file or directory），测试目标无法生成。
- 影响：README 承诺的测试体系无从运行，构建流程与质量保证环节均与文档不符。

## 6. 批量请求处理方式不符描述
- 说明：README.md:20-23 宣称“批量请求并行处理”。
- 现状：`include/jsonrpc/impl/method_registry.ipp:68-85` 中 `MethodRegistry::invoke_batch` 只是顺序遍历请求并串行调用 `invoke`，没有任何并行调度或线程池逻辑。
- 影响：README 对性能特性的描述与真实实现不符，用户期待的并行能力并不存在。

## 7. 示例文档引用缺失
- 说明：README.md:191 提到 `examples/README.md` 提供示例说明。
- 现状：仓库中不存在该文件，进一步加剧了示例文档与实现的缺失问题。

---

以上差异会导致 README 中的多项承诺（示例、构建方式、依赖要求、性能特性与测试流程）均无法兑现，需要依次补齐示例源码、完善 CMake 构建脚本、修复模板定义、补全测试以及更新文档描述，以恢复工程可信度。
