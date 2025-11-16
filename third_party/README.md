# 第三方库目录

此目录包含 JsonRPC 项目依赖的第三方库。

## Boost 1.83.0

### 下载方式

本项目提供了自动下载脚本，无需手动安装系统 Boost。

**Linux / macOS**:
```bash
bash scripts/download_boost.sh
```

**Windows**:
```batch
scripts\download_boost.bat
```

### 目录结构

下载完成后，目录结构如下：

```
third_party/
├── README.md           # 本文件
└── boost/              # Boost 1.83.0 源码（自动下载）
    ├── boost/          # Boost 头文件目录
    │   ├── asio/
    │   ├── beast/
    │   ├── json/
    │   ├── system/
    │   └── ...
    ├── libs/           # Boost 库源码
    └── ...
```

### 使用的 Boost 组件

本项目使用以下 Boost 组件（全部为 header-only，无需编译）：

- **Boost.JSON** - JSON 解析和序列化
- **Boost.Asio** - 异步 I/O 和网络编程
- **Boost.Beast** - HTTP 服务器和客户端
- **Boost.System** - 错误处理（Header-Only 模式）

### 版本要求

- **最低版本**: Boost 1.83.0
- **推荐版本**: Boost 1.83.0

### CMake 集成

项目的 `CMakeLists.txt` 会自动检测：

1. **优先使用本地 Boost**：如果 `third_party/boost/` 存在，使用本地 Boost
2. **回退到系统 Boost**：如果本地不存在，尝试使用系统安装的 Boost

CMake 配置时会显示使用的 Boost 来源：

```
-- 检测到本地 Boost: /path/to/project/third_party/boost
-- 本地 Boost 版本: 1.83.0
-- 使用本地 Boost 1.83.0
  包含目录: /path/to/project/third_party/boost
  模式: Header-Only
```

或

```
-- 未找到本地 Boost (/path/to/project/third_party/boost)
-- 提示: 运行 scripts/download_boost.sh 下载本地 Boost
-- 尝试查找系统 Boost 1.83.0...
-- 使用系统 Boost 1.83.0
  包含目录: /usr/include
  库文件: /usr/lib/x86_64-linux-gnu/libboost_system.so;...
```

### 为什么不提交到仓库？

Boost 源码包较大（约 130MB），提交到 Git 仓库会显著增加仓库体积。因此：

- `third_party/boost/` 已添加到 `.gitignore`
- 用户需要运行下载脚本获取 Boost
- CI/CD 环境可以缓存 `third_party/` 目录以加速构建

### 手动下载（可选）

如果自动下载脚本无法使用，可以手动下载：

1. 访问 Boost 官网：https://www.boost.org/users/history/version_1_83_0.html
2. 下载 `boost_1_83_0.tar.gz` 或 `boost_1_83_0.zip`
3. 解压到 `third_party/boost/` 目录
4. 确保 `third_party/boost/boost/version.hpp` 文件存在

### 故障排除

**问题 1: 下载失败**
- 检查网络连接
- 尝试使用镜像地址（脚本会自动尝试）
- 手动下载后解压到 `third_party/boost/`

**问题 2: CMake 找不到 Boost**
- 确认 `third_party/boost/boost/version.hpp` 存在
- 运行 `cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON` 查看详细日志
- 检查 Boost 版本是否 >= 1.83.0

**问题 3: 编译错误**
- 确认使用的是 Boost 1.83.0（而非其他版本）
- 确认 C++ 标准为 C++11 或更高
- 检查编译器版本是否支持 C++11

## 许可证

Boost 库使用 Boost Software License 1.0，详见：
https://www.boost.org/LICENSE_1_0.txt

## 作者

脚本和集成配置：无事情小神仙
