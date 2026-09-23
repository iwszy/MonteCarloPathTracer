#pragma once

#include <unordered_map>
#include <glm/glm.hpp>
#include "bvh.hpp"
#include "camera.hpp"
#include "light.hpp"

/// <summary>
/// 场景类，包含场景的BVH以及光源信息
/// </summary>
class Scene {
public:
	Scene();
	~Scene();

	/// <summary>
	/// 从xml中加载场景信息
	/// </summary>
	/// <param name="filepath">xml文件位置</param>
	/// <param name="model">模型类</param>
	/// <returns>相机类</returns>
	Camera* loadXML(const std::string& filepath, Model* model);
	/// <summary>
	/// 构建BVH
	/// </summary>
	/// <param name="model">模型类</param>
	void buildBVH(Model* model);
	/// <summary>
	/// 计算射线与场景的交点
	/// </summary>
	/// <param name="ray">要求交的射线</param>
	/// <param name="intersection">交点，若射线未击中场景则无效</param>
	/// <returns>射线是否与场景有交点</returns>
	bool hit(Ray ray, Intersection& intersection) const;

	/// <summary>
	/// 根据材质名称获取使用该材质的光源
	/// </summary>
	/// <param name="materialName">材质名称</param>
	/// <returns>使用该材质的光源</returns>
	Light* getLight(const std::string& materialName) const { return m_lightMap.at(materialName); }
	/// <summary>
	/// 获取场景中光源数量是否为0
	/// </summary>
	/// <returns>场景中光源数量是否为0</returns>
	bool isLightEmpty() const { return m_lights.empty(); }
	/// <summary>
	/// 获取背景颜色
	/// </summary>
	/// <returns>背景颜色</returns>
	glm::vec3 getBackground() const { return m_background; }
	/// <summary>
	/// 获取场景中的光源数组
	/// </summary>
	/// <returns>场景中的光源数组</returns>
	std::vector<Light*>& getLights() { return m_lights; }

private:
	/// <summary>
	/// 光源数组
	/// </summary>
	std::vector<Light*> m_lights;
	/// <summary>
	/// 材质名称到光源的映射
	/// </summary>
	std::unordered_map<std::string, Light*> m_lightMap;
	/// <summary>
	/// BVH
	/// </summary>
	std::unique_ptr<BVH> m_bvh;
	/// <summary>
	/// 背景颜色
	/// </summary>
	glm::vec3 m_background;
};
