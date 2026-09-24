cmake_minimum_required(VERSION 3.16)

project(MonteCarloPathTracer
    VERSION 1.0.0
    DESCRIPTION "CPU Monte Carlo path tracer: SAH BVH, NEE + MIS, Owen-scrambled Sobol, HDR accumulation with ACES / sRGB output"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# 语言标准与默认构建类型
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器（Makefiles / Ninja）下默认 Release，避免忘记 -DCMAKE_BUILD_TYPE 而得到 -O0
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "构建类型" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

# 可选：本机最大化优化。还能再快约 6%~11%，但牺牲二进制可移植性，且不同机器/编译器的
# 浮点结果不再逐字节一致，故默认关闭。
option(MCPT_NATIVE_OPT "启用 -O3 -march=native（MSVC 为 /arch:AVX2）" OFF)

# ---------------------------------------------------------------------------
# 目标：单个可执行文件
# ---------------------------------------------------------------------------
set(MCPT_SOURCES
    src/bvh.cpp
    src/camera.cpp
    src/intersection.cpp
    src/light.cpp
    src/main.cpp
    src/model.cpp
    src/pathTracer.cpp
    src/sampler.cpp
    src/scene.cpp
    src/texture.cpp
    include/xml/tinyxml2.cpp
)

set(MCPT_HEADERS
    src/boundingbox.hpp
    src/bvh.hpp
    src/camera.hpp
    src/constant.hpp
    src/intersection.hpp
    src/light.hpp
    src/material.hpp
    src/model.hpp
    src/pathTracer.hpp
    src/ray.hpp
    src/sampler.hpp
    src/scene.hpp
    src/texture.hpp
)

add_executable(MonteCarloPathTracer ${MCPT_SOURCES} ${MCPT_HEADERS})

# 项目源码在 src/；include/ 下是自己的第三方库（glm / stb / tinyxml2），标记为 SYSTEM
# 以屏蔽来自它们的编译告警。
target_include_directories(MonteCarloPathTracer PRIVATE src)
target_include_directories(MonteCarloPathTracer SYSTEM PRIVATE include)

find_package(Threads REQUIRED)
target_link_libraries(MonteCarloPathTracer PRIVATE Threads::Threads)

if(MSVC)
    # 同一项目内并行编译（多核下明显更快）
    target_compile_options(MonteCarloPathTracer PRIVATE /MP)
endif()

if(MCPT_NATIVE_OPT)
    if(MSVC)
        target_compile_options(MonteCarloPathTracer PRIVATE /arch:AVX2)
    else()
        target_compile_options(MonteCarloPathTracer PRIVATE -O3 -march=native)
    endif()
endif()

# 程序按相对路径读 models/ 并写 results/，因此调试时把工作目录设为仓库根目录
set_target_properties(MonteCarloPathTracer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
