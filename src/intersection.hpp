#pragma once

#include <glm/glm.hpp>
#include "material.hpp"
#include "sampler.hpp"

/// <summary>
/// 交点类，记录交点所需参数并提供BRDF功能
/// </summary>
class Intersection {
public:
	/// <summary>
	/// 交点位置
	/// </summary>
	glm::vec3 point;
	/// <summary>
	/// 交点法线
	/// </summary>
	glm::vec3 normal;
	/// <summary>
	/// 交点纹理坐标
	/// </summary>
	glm::vec2 uv;
	/// <summary>
	/// 射线在该点的时间值
	/// </summary>
	float t;
	/// <summary>
	/// 交点所在面的材质
	/// </summary>
	const Material* material = nullptr;
	/// <summary>
	/// 交点所在面的索引
	/// </summary>
	int id;

	/// <summary>
	/// 判断给定的出射方向是否符合BRDF分布，并计算PDF与BRDF值
	/// </summary>
	/// <param name="wo">入射方向</param>
	/// <param name="wi">出射方向</param>
	/// <param name="brdfVal">BRDF值</param>
	/// <param name="pdf">BRDF在出射方向上的PDF</param>
	/// <returns>出射方向是否符合BRDF分布，即pdf是否>=0</returns>
	bool brdf(glm::vec3 wo, glm::vec3 wi, glm::vec3& brdfVal, float& pdf);
	/// <summary>
	/// 根据材质的BRDF分布采样新方向并计算PDF与BRDF值
	/// </summary>
	/// <param name="wo">入射方向</param>
	/// <param name="wi">出射方向</param>
	/// <param name="pdf">BRDF的PDF</param>
	/// <param name="sampler">采样器</param>
	/// <returns>BRDF值</returns>
	glm::vec3 brdf(glm::vec3 wo, glm::vec3& wi, float& pdf, Sampler* sampler);
	/// <summary>
	/// 设置法线并计算法线坐标系
	/// </summary>
	/// <param name="n">法线</param>
	/// 混合材质高光强度 0..1：F0 = mix(0.04, Ks, blend)。纯镜面分支不受影响，仍用 F0 = Ks
	static void setSpecularBlend(float blend) { s_specularBlend = glm::clamp(blend, 0.f, 1.f); }
	static float s_specularBlend;

	void setNormal(glm::vec3 n);
private:
	/// <summary>
	/// 从法线坐标系向世界坐标系的转换矩阵
	/// </summary>
	glm::mat3 m_transform = glm::mat3(0);
	/// <summary>
	/// 从世界坐标系向法线坐标系的转换矩阵
	/// </summary>
	glm::mat3 m_transposeTransform = glm::mat3(0);

	/// <summary>
	/// 根据漫反射BRDF采样新方向并计算PDF与BRDF值
	/// </summary>
	/// <param name="wi">出射方向</param>
	/// <param name="pdf">BRDF的PDF</param>
	/// <param name="sampler">采样器</param>
	/// <returns>BRDF值</returns>
	glm::vec3 diffuseReflect(glm::vec3& wi, float& pdf, Sampler* sampler) const;
	/// <summary>
	/// 根据镜面反射BRDF采样新方向并计算PDF与BRDF值
	/// </summary>
	/// <param name="wo">入射方向</param>
	/// <param name="wi">出射方向</param>
	/// <param name="pdf">BRDF的PDF</param>
	/// <param name="sampler">采样器</param>
	/// <returns>BRDF值</returns>
	glm::vec3 specularReflect(glm::vec3 wo, glm::vec3& wi, float& pdf, Sampler* sampler);

	/// <summary>
	/// 求"漫反射 + 镜面反射"混合材质的完整 BRDF 值与采样 PDF
	/// BRDF 取两项之和（Phong 模型两项同时存在），PDF 取整个采样过程的边缘密度
	/// （两个 lobe 的密度按权重求和）。求值路径与采样路径必须共用此函数，
	/// 否则光源采样与 BSDF 采样估计的积分对象不一致，MIS 加权在数学上不成立。
	/// </summary>
	/// <param name="wo">入射方向</param>
	/// <param name="wi">出射方向</param>
	/// <param name="pdf">边缘采样 PDF</param>
	/// <returns>完整 BRDF 值</returns>
	glm::vec3 evaluateMixed(glm::vec3 wo, glm::vec3 wi, float& pdf) const;
	/// <summary>
	/// 根据GGX分布计算微表面法线分布项
	/// </summary>
	/// <param name="hDotN">半程向量与法线的点积</param>
	/// <returns>D值</returns>
	float ggx(float hDotN) const;
	/// <summary>
	/// 计算菲涅尔项
	/// </summary>
	/// <param name="f0">垂直入射时的反射率</param>
	/// <param name="iDotN">出射方向与法线的点积</param>
	/// <returns>菲涅尔项</returns>
	glm::vec3 schlickFresnel(glm::vec3 f0, float iDotN) const;
	/// <summary>
	/// 根据smith GGX模型计算几何遮蔽与阴影函数项
	/// </summary>
	/// <param name="wDotN">射线与法线的点积</param>
	/// <returns>几何遮蔽与阴影函数项</returns>
	float smithGGX(float wDotN) const;
};
