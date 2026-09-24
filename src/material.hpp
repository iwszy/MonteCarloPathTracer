#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "constant.hpp"
#include "texture.hpp"

/// <summary>
/// 材质类型枚举
/// </summary>
enum MaterialType : uint8_t {
	/// <summary>
	/// 纯漫反射材质，不包含镜面反射项
	/// </summary>
	DIFFUSE,
	/// <summary>
	/// 漫反射与镜面反射混合材质，既有漫反射项又有镜面反射项
	/// </summary>
	DIFFUSE_SPECULAR,
	/// <summary>
	/// 纯镜面反射材质，不包含漫反射项
	/// </summary>
	SPECULAR,
	/// <summary>
	/// 光源
	/// </summary>
	LIGHT
};

/// <summary>
/// 材质类，只处理不透明材质
/// </summary>
class Material {
public:
	/// <summary>
	/// 材质名称
	/// </summary>
	std::string name;
	/// <summary>
	/// 材质的漫反射项的基础颜色
	/// </summary>
	glm::vec3 diffuse;
	/// <summary>
	/// 材质的镜面反射项的基础颜色
	/// </summary>
	glm::vec3 specular;
	/// <summary>
	/// 材质的透明度，本项目虽然记录此项，但不处理透明材质
	/// </summary>
	glm::vec3 transmission;
	/// <summary>
	/// 材质所对应的光源的radiance，非光源材质此参数无意义
	/// </summary>
	glm::vec3 radiance;
	/// <summary>
	/// 高光指数
	/// </summary>
	float shininess;
	/// <summary>
	/// 折射率，本项目虽然记录此项，但不处理会进行折射的材质
	/// </summary>
	float ior;
	/// <summary>
	/// 粗糙度的平方，通过经验公式由高光指数转换而来
	/// </summary>
	float a2;
	/// <summary>
	/// 漫反射率，指明材质有多大概率进行漫反射
	/// </summary>
	float diffuseRate;
	/// <summary>
	/// 材质类型
	/// </summary>
	MaterialType type;
	/// <summary>
	/// 纹理
	/// </summary>
	Texture* texture;
	/// <summary>
	/// 使用该材质的面的数组，只在光源创建时有用，渲染前会清空
	/// </summary>
	std::vector<int> faces;

	Material() {
		diffuse = glm::vec3(1.f);
		specular = glm::vec3(0.f);
		transmission = glm::vec3(1.f);
		radiance = glm::vec3(0.f);
		shininess = 0;
		a2 = 1;
		ior = 1;
		diffuseRate = 1;
		type = DIFFUSE;
		texture = nullptr;
	}

	/// <summary>
	/// 根据纹理图片路径加载纹理
	/// </summary>
	/// <param name="filepath">纹路图片路径</param>
	void setMapKd(const std::string& filepath) {
		texture = new Texture(filepath);
	}
	/// <summary>
	/// 获取材质的漫反射颜色
	/// </summary>
	/// <param name="uv">纹理坐标</param>
	/// <returns>材质的漫反射颜色</returns>
	glm::vec3 getDiffuse(glm::vec2 uv) const {
		//没有纹理直接返回diffuse，若存在纹理而漫反射项为0则返回纹理颜色，否则返回两者相乘结果
		if (texture == nullptr) {
			return diffuse;
		}
		if (glm::dot(diffuse, diffuse) < EPSILON) {
			return texture->sample(uv);
		}
		return texture->sample(uv) * diffuse;
	}
	/// <summary>
	/// 计算材质的类型以及相关参数
	/// </summary>
	void calculateType() {
		if (glm::length(specular) < EPSILON) {
			type = DIFFUSE;
		}else if (glm::length(diffuse) < EPSILON) {
			type = SPECULAR;
		}else {
			type = DIFFUSE_SPECULAR;
			float kd = glm::dot(diffuse, glm::vec3(1)), ks = glm::dot(specular, glm::vec3(1));
			diffuseRate = kd / (kd + ks);
		}
		a2 = glm::clamp(2 / (shininess + 2), 0.0001f, 1.f);
	}
	/// <summary>
	/// 是否按理想 δ 镜面处理：仅对纯镜面材质生效（含漫反射分量的混合材质仍走 GGX 叶瓣）
	/// </summary>
	bool isDeltaSpecular() const {
		return type == SPECULAR && a2 < DELTA_SPECULAR_A2;
	}
};
