#pragma once

/// <summary>
/// 极小值，用于误差判断
/// </summary>
constexpr float EPSILON = 1e-5f;
/// <summary>
/// 2π
/// </summary>
constexpr float DOUBLE_PI = 6.2831853071795864f;
/// <summary>
/// 1 / π
/// </summary>
constexpr float INV_PI = 0.31830988618379067f;
/// <summary>
/// 默认曝光系数：物理辐射亮度先乘曝光，再经 ACES 色调映射与 sRGB 编码写成 8bit 图片
/// （原先分散在 NEE/BSDF 两条路径里的 800 与 40 两个魔数已删除，这里只留一个统一曝光量）
/// 可在 xml 的 <camera exposure="..."/> 中逐场景覆盖，也可用 PathTracer::setExposure() 调整
/// </summary>
constexpr float DEFAULT_EXPOSURE = 300.f;

/// <summary>
/// 极光滑镜面的判定阈值（a2 为微表面平方粗糙度）。
/// a2 小于该值时叶瓣半宽已不足约 1.8 度，按理想 δ 镜面处理：
/// 反射方向唯一确定，可消除 GGX 叶瓣抖动在镜面像素上与场景对比度成正比的巨大方差
/// </summary>
constexpr float DELTA_SPECULAR_A2 = 1e-3f;

/// <summary>
/// δ 镜面的密度占位值：δ 分布不是普通密度函数，这里取一个远大于任何光源 pdf 的常数，
/// 使 MIS 在“镜面反射后命中光源”时把权重几乎全部交给 BSDF 采样这一路
/// </summary>
constexpr float DELTA_SPECULAR_PDF = 1e8f;
