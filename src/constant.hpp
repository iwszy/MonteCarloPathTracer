#pragma once

/// <summary>
/// 极小值，用于误差判断
/// </summary>
constexpr float EPSILON = 1e-5f;
/// <summary>
/// π
/// </summary>
constexpr float PI = 3.1415926535897932f;
/// <summary>
/// 2π
/// </summary>
constexpr float DOUBLE_PI = 6.2831853071795864f;
/// <summary>
/// 1 / π
/// </summary>
constexpr float INV_PI = 0.31830988618379067f;
/// <summary>
/// 1 / 2π
/// </summary>
constexpr float INV_DOUBLE_PI = 0.159154943091895336f;
/// <summary>
/// 默认曝光系数：物理辐射亮度先乘曝光，再经 ACES 色调映射与 sRGB 编码写成 8bit 图片
/// （原先分散在 NEE/BSDF 两条路径里的 800 与 40 两个魔数已删除，这里只留一个统一曝光量）
/// 可在 xml 的 <camera exposure="..."/> 中逐场景覆盖，也可用 PathTracer::setExposure() 调整
/// </summary>
constexpr float DEFAULT_EXPOSURE = 300.f;
