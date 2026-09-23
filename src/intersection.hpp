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
	Material material;
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
	/// <param name="sampler">采样器</param>
	/// <returns>出射方向是否符合BRDF分布，即pdf是否>=0</returns>
	bool brdf(glm::vec3 wo, glm::vec3 wi, glm::vec3& brdfVal, float& pdf, Sampler* sampler);
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
