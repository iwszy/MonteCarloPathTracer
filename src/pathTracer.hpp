#pragma once

#include <mutex>
#include "scene.hpp"
#include "camera.hpp"
#include "intersection.hpp"
#include "sampler.hpp"

/// <summary>
/// 最大递归深度
/// </summary>
constexpr int MAX_DEPTH = 8;

/// <summary>
/// 路径追踪核心类，实现路径追踪
/// </summary>
class PathTracer {
public:
	PathTracer(Scene* scene, Camera* camera);
	~PathTracer();

	/// <summary>
	/// 渲染整个场景
	/// </summary>
	void render();
	void bilateralFilter(int k, float sigmaD, float sigmaR);
	/// <summary>
	/// 保存渲染完成后的渲染图片
	/// </summary>
	/// <param name="modelName">模型名称</param>
	void save(std::string modelName) const;
private:
	/// <summary>
	/// 场景类
	/// </summary>
	Scene* m_scene;
	/// <summary>
	/// 照相机类
	/// </summary>
	Camera* m_camera;
	/// <summary>
	/// 采样器类
	/// </summary>
	Sampler* m_sampler;
	
	/// <summary>
	/// 每像素采样的光线数
	/// </summary>
	int m_spp;
	/// <summary>
	/// 渲染图像的存储位置
	/// </summary>
	unsigned char* m_image;

	/// <summary>
	/// 渲染像素块中像素
	/// </summary>
	/// <param name="x">像素块的x坐标</param>
	/// <param name="y">像素块的y坐标</param>
	void renderPixel(int x, int y);
	/// <summary>
	/// 跟踪发射的光线
	/// </summary>
	/// <param name="ray">正在追踪的光线</param>
	/// <param name="depth">当前深度</param>
	/// <returns>这条光线击中位置的颜色值</returns>
	glm::vec3 trace(Ray ray, int depth);
	/// <summary>
	/// 直接光照计算
	/// </summary>
	///	<param name="wo">光线入射方向</param>
	/// <param name="intersection">光线与场景的交点</param>
	/// <returns>直接光照的计算结果</returns>
	glm::vec3 sampleDirectLight(glm::vec3 wo, Intersection& intersection);
	/// <summary>
	/// 使用幂启发式计算两个PDF的MIS权重，幂指数为2
	/// </summary>
	/// <param name="pdf1">第一个PDF</param>
	/// <param name="pdf2">第二个PDF</param>
	/// <returns>MIS权重</returns>
	float powerHeuristic(float pdf1, float pdf2);
};