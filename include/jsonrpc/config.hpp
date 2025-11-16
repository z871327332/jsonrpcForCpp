#pragma once

// C++11 版本检查
#if __cplusplus < 201103L
#  error "This library requires C++11 or later"
#endif

// Header-only 模式配置
#ifdef JSONRPC_HEADER_ONLY
#  define JSONRPC_DECL
#else
#  ifdef _WIN32
#    ifdef JSONRPC_BUILD_SHARED
#      define JSONRPC_DECL __declspec(dllexport)
#    elif defined(JSONRPC_USE_SHARED)
#      define JSONRPC_DECL __declspec(dllimport)
#    else
#      define JSONRPC_DECL
#    endif
#  else
#    define JSONRPC_DECL
#  endif
#endif

// Boost.Asio/JSON Header-Only 模式
#ifndef BOOST_ASIO_HEADER_ONLY
#define BOOST_ASIO_HEADER_ONLY
#endif

#ifndef BOOST_JSON_HEADER_ONLY
#define BOOST_JSON_HEADER_ONLY
#endif

// 编译器版本检查（可选）
#if defined(_MSC_VER)
#  if _MSC_VER < 1900  // Visual Studio 2015
#    error "This library requires Visual Studio 2015 or later for full C++11 support"
#  endif
#elif defined(__GNUC__)
#  if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#    error "This library requires GCC 4.8 or later for full C++11 support"
#  endif
#elif defined(__clang__)
#  if __clang_major__ < 3 || (__clang_major__ == 3 && __clang_minor__ < 3)
#    error "This library requires Clang 3.3 or later for full C++11 support"
#  endif
#endif
