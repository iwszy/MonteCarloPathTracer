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
/// 默认曝光系数：物理辐射亮度先乘曝光，再经 ACES 色调映射与 sRGB 编码写成 8bit 图片
/// （原先分散在 NEE/BSDF 两条路径里的 800 与 40 两个魔数已删除，这里只留一个统一曝光量）
/// 可在运行时用 PathTracer::setExposure() 调整
/// </summary>
constexpr float DEFAULT_EXPOSURE = 300.f;

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
	/// <summary>
	/// 设置曝光系数（渲染完成后只要重新 developImage() 即可换曝光出图，无需重新渲染）
	/// </summary>
	void setExposure(float exposure) { m_exposure = exposure; }
	/// <summary>
	/// 获取当前曝光系数
	/// </summary>
	float getExposure() const { return m_exposure; }
	/// <summary>
	/// 把线性 HDR 缓冲显影为 8bit 显示图：曝光 -> ACES 色调映射 -> sRGB 编码
	/// </summary>
	void developImage() const;
	/// <summary>
	/// 输出线性 HDR 结果（Radiance .hdr），便于后期自行调整
	/// </summary>
	void saveHDR(const std::string& modelName) const;
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
	/// 线性 HDR 累加缓冲（3 个 float / 像素）：渲染结果先无损累加到这里
	/// </summary>
	float* m_hdrImage;
	/// <summary>
	/// 相机曝光系数（物理辐射亮度 -> 色调映射输入）
	/// </summary>
	float m_exposure;

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
	/// <param name="bsdfPDF">生成该光线的 BSDF 采样 PDF，用于 BSDF 策略命中光源时计算 MIS 权重</param>
	/// <returns>这条光线击中位置的颜色值</returns>
	glm::vec3 trace(Ray ray, int depth, float bsdfPDF = 0.f);
	/// <summary>
	/// 直接光照计算
	/// </summary>
	///	<param name="wo">光线入射方向</param>
	/// <param name="intersection">光线与场景的交点</param>
	/// <returns>直接光照的计算结果</returns>
	glm::vec3 sampleDirectLight(glm::vec3 wo, Intersection& intersection);

	/// <summary>
	/// 光源采样策略对"命中该光源"这一方向的立体角 PDF，即 d^2 / (A * cosθ)
	/// </summary>
	/// <param name="ray">命中光源的光线，其原点即上一个顶点</param>
	/// <param name="intersection">光源面上的交点</param>
	/// <returns>该方向上的立体角 PDF</returns>
	float sampleLightPdf(const Ray& ray, const Intersection& intersection) const;
	/// <summary>
	/// 使用幂启发式计算两个PDF的MIS权重，幂指数为2
	/// </summary>
	/// <param name="pdf1">第一个PDF</param>
	/// <param name="pdf2">第二个PDF</param>
	/// <returns>MIS权重</returns>
	float powerHeuristic(float pdf1, float pdf2);
};