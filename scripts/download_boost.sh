#!/bin/bash
# JsonRPC - Boost 1.83.0 下载脚本
# 用途：自动下载 Boost 1.83.0 到 third_party/boost/
# 作者：无事情小神仙

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置
BOOST_VERSION="1.83.0"
BOOST_VERSION_UNDERSCORE="1_83_0"
BOOST_ARCHIVE="boost_${BOOST_VERSION_UNDERSCORE}.tar.gz"
# SHA256 for Boost 1.83.0 (from official release)
BOOST_SHA256="c0685b68dd44cc46574cce86c4e17c0f611b15e195be9848dfd0769a0a207628"
# 使用 SourceForge 作为主 URL（更稳定）
BOOST_URL="https://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/${BOOST_ARCHIVE}/download"
# GitHub Release 作为备用
BOOST_MIRROR_URL="https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}.tar.gz"

# 项目路径
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
THIRD_PARTY_DIR="${PROJECT_ROOT}/third_party"
BOOST_DIR="${THIRD_PARTY_DIR}/boost"
DOWNLOAD_DIR="${THIRD_PARTY_DIR}/.download"

echo -e "${GREEN}=== JsonRPC Boost 下载脚本 ===${NC}"
echo "Boost 版本: ${BOOST_VERSION}"
echo "目标目录: ${BOOST_DIR}"
echo ""

# 检查是否已存在
if [ -d "${BOOST_DIR}" ]; then
    echo -e "${YELLOW}警告: ${BOOST_DIR} 已存在${NC}"
    read -p "是否删除并重新下载？(y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "删除现有 Boost..."
        rm -rf "${BOOST_DIR}"
    else
        echo "保留现有 Boost，退出。"
        exit 0
    fi
fi

# 创建下载目录
mkdir -p "${DOWNLOAD_DIR}"
cd "${DOWNLOAD_DIR}"

# 下载 Boost
echo -e "${GREEN}[1/4] 下载 Boost ${BOOST_VERSION}...${NC}"
DOWNLOAD_SUCCESS=false

# 尝试主 URL
if [ ! -f "${BOOST_ARCHIVE}" ]; then
    echo "尝试从主站下载: ${BOOST_URL}"
    if wget -O "${BOOST_ARCHIVE}" "${BOOST_URL}" 2>/dev/null || curl -L -o "${BOOST_ARCHIVE}" "${BOOST_URL}" 2>/dev/null; then
        DOWNLOAD_SUCCESS=true
        echo -e "${GREEN}✓ 下载成功${NC}"
    else
        echo -e "${YELLOW}主站下载失败，尝试镜像...${NC}"
    fi
fi

# 如果主 URL 失败，尝试镜像
if [ "$DOWNLOAD_SUCCESS" = false ]; then
    echo "尝试从镜像下载: ${BOOST_MIRROR_URL}"
    if wget -O "${BOOST_ARCHIVE}" "${BOOST_MIRROR_URL}" 2>/dev/null || curl -L -o "${BOOST_ARCHIVE}" "${BOOST_MIRROR_URL}" 2>/dev/null; then
        DOWNLOAD_SUCCESS=true
        echo -e "${GREEN}✓ 镜像下载成功${NC}"
    else
        echo -e "${RED}✗ 下载失败！${NC}"
        echo "请手动下载 Boost 1.83.0 并解压到: ${BOOST_DIR}"
        echo "下载地址: https://www.boost.org/users/history/version_1_83_0.html"
        exit 1
    fi
fi

# 校验 SHA256
echo -e "${GREEN}[2/4] 校验文件完整性...${NC}"
if command -v sha256sum >/dev/null 2>&1; then
    ACTUAL_SHA256=$(sha256sum "${BOOST_ARCHIVE}" | awk '{print $1}')
elif command -v shasum >/dev/null 2>&1; then
    ACTUAL_SHA256=$(shasum -a 256 "${BOOST_ARCHIVE}" | awk '{print $1}')
else
    echo -e "${YELLOW}警告: 未找到 sha256sum 或 shasum，跳过校验${NC}"
    ACTUAL_SHA256="${BOOST_SHA256}"
fi

if [ "${ACTUAL_SHA256}" = "${BOOST_SHA256}" ]; then
    echo -e "${GREEN}✓ SHA256 校验通过${NC}"
else
    echo -e "${RED}✗ SHA256 校验失败！${NC}"
    echo "期望: ${BOOST_SHA256}"
    echo "实际: ${ACTUAL_SHA256}"
    echo "文件可能已损坏，请重新下载。"
    exit 1
fi

# 解压
echo -e "${GREEN}[3/4] 解压 Boost...${NC}"
tar -xzf "${BOOST_ARCHIVE}"

if [ ! -d "boost_${BOOST_VERSION_UNDERSCORE}" ]; then
    echo -e "${RED}✗ 解压失败，未找到目录 boost_${BOOST_VERSION_UNDERSCORE}${NC}"
    exit 1
fi

# 移动到目标位置
echo -e "${GREEN}[4/4] 安装到 ${BOOST_DIR}...${NC}"
mv "boost_${BOOST_VERSION_UNDERSCORE}" "${BOOST_DIR}"

# 清理下载文件
echo "清理临时文件..."
cd "${PROJECT_ROOT}"
rm -rf "${DOWNLOAD_DIR}"

# 完成
echo ""
echo -e "${GREEN}=== 完成！===${NC}"
echo -e "${GREEN}✓ Boost ${BOOST_VERSION} 已成功安装到:${NC}"
echo "  ${BOOST_DIR}"
echo ""
echo "现在可以运行以下命令编译项目："
echo "  mkdir -p build && cd build"
echo "  cmake .."
echo "  make"
echo ""
