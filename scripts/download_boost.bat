@echo off
REM JsonRPC - Boost 1.83.0 下载脚本 (Windows)
REM 用途：自动下载 Boost 1.83.0 到 third_party\boost\
REM 作者：无事情小神仙

setlocal enabledelayedexpansion

REM 配置
set BOOST_VERSION=1.83.0
set BOOST_VERSION_UNDERSCORE=1_83_0
set BOOST_ARCHIVE=boost_%BOOST_VERSION_UNDERSCORE%.tar.gz
REM SHA256 for Boost 1.83.0 (from official release)
set BOOST_SHA256=c0685b68dd44cc46574cce86c4e17c0f611b15e195be9848dfd0769a0a207628
REM 使用 SourceForge 作为主 URL（更稳定）
set BOOST_URL=https://sourceforge.net/projects/boost/files/boost/%BOOST_VERSION%/%BOOST_ARCHIVE%/download
REM GitHub Release 作为备用
set BOOST_MIRROR_URL=https://github.com/boostorg/boost/releases/download/boost-%BOOST_VERSION%/boost-%BOOST_VERSION%.tar.gz

REM 路径设置
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set THIRD_PARTY_DIR=%PROJECT_ROOT%\third_party
set BOOST_DIR=%THIRD_PARTY_DIR%\boost
set DOWNLOAD_DIR=%THIRD_PARTY_DIR%\.download

echo === JsonRPC Boost 下载脚本 ===
echo Boost 版本: %BOOST_VERSION%
echo 目标目录: %BOOST_DIR%
echo.

REM 检查是否已存在
if exist "%BOOST_DIR%" (
    echo 警告: %BOOST_DIR% 已存在
    set /p REPLY="是否删除并重新下载？(y/N): "
    if /i "!REPLY!"=="y" (
        echo 删除现有 Boost...
        rmdir /s /q "%BOOST_DIR%"
    ) else (
        echo 保留现有 Boost，退出。
        exit /b 0
    )
)

REM 创建下载目录
if not exist "%DOWNLOAD_DIR%" mkdir "%DOWNLOAD_DIR%"
cd /d "%DOWNLOAD_DIR%"

REM 检查下载工具
where curl >nul 2>&1
if %errorlevel% equ 0 (
    set DOWNLOADER=curl
) else (
    where powershell >nul 2>&1
    if %errorlevel% equ 0 (
        set DOWNLOADER=powershell
    ) else (
        echo 错误: 未找到 curl 或 powershell，无法下载
        exit /b 1
    )
)

REM 下载 Boost
echo [1/4] 下载 Boost %BOOST_VERSION%...
if not exist "%BOOST_ARCHIVE%" (
    echo 尝试下载: %BOOST_URL%

    if "%DOWNLOADER%"=="curl" (
        curl -L -o "%BOOST_ARCHIVE%" "%BOOST_URL%"
    ) else (
        powershell -Command "Invoke-WebRequest -Uri '%BOOST_URL%' -OutFile '%BOOST_ARCHIVE%'"
    )

    if !errorlevel! neq 0 (
        echo 主站下载失败，尝试镜像...
        if "%DOWNLOADER%"=="curl" (
            curl -L -o "%BOOST_ARCHIVE%" "%BOOST_MIRROR_URL%"
        ) else (
            powershell -Command "Invoke-WebRequest -Uri '%BOOST_MIRROR_URL%' -OutFile '%BOOST_ARCHIVE%'"
        )

        if !errorlevel! neq 0 (
            echo 错误: 下载失败！
            echo 请手动下载 Boost 1.83.0 并解压到: %BOOST_DIR%
            echo 下载地址: https://www.boost.org/users/history/version_1_83_0.html
            exit /b 1
        )
    )

    echo √ 下载成功
)

REM 校验 SHA256 (需要 PowerShell 4.0+)
echo [2/4] 校验文件完整性...
powershell -Command "$hash = (Get-FileHash -Algorithm SHA256 '%BOOST_ARCHIVE%').Hash.ToLower(); if ($hash -eq '%BOOST_SHA256%') { exit 0 } else { exit 1 }"
if %errorlevel% equ 0 (
    echo √ SHA256 校验通过
) else (
    echo 警告: SHA256 校验失败或无法校验，继续解压...
)

REM 解压 (需要 tar 或 7-Zip)
echo [3/4] 解压 Boost...

REM 尝试使用 Windows 10+ 内置的 tar
where tar >nul 2>&1
if %errorlevel% equ 0 (
    tar -xzf "%BOOST_ARCHIVE%"
) else (
    REM 尝试使用 7-Zip
    where 7z >nul 2>&1
    if %errorlevel% equ 0 (
        7z x "%BOOST_ARCHIVE%" -so | 7z x -si -ttar
    ) else (
        echo 错误: 未找到 tar 或 7-Zip，无法解压
        echo 请安装 7-Zip 或使用 Windows 10+ 自带的 tar
        exit /b 1
    )
)

if not exist "boost_%BOOST_VERSION_UNDERSCORE%" (
    echo 错误: 解压失败，未找到目录 boost_%BOOST_VERSION_UNDERSCORE%
    exit /b 1
)

REM 移动到目标位置
echo [4/4] 安装到 %BOOST_DIR%...
move "boost_%BOOST_VERSION_UNDERSCORE%" "%BOOST_DIR%"

REM 清理下载文件
echo 清理临时文件...
cd /d "%PROJECT_ROOT%"
rmdir /s /q "%DOWNLOAD_DIR%"

REM 完成
echo.
echo === 完成！===
echo √ Boost %BOOST_VERSION% 已成功安装到:
echo   %BOOST_DIR%
echo.
echo 现在可以运行以下命令编译项目:
echo   mkdir build ^&^& cd build
echo   cmake ..
echo   cmake --build .
echo.

endlocal
