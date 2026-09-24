#pragma once

#include <mutex>
#include "scene.hpp"
#include "camera.hpp"
#include "intersection.hpp"
#include "sampler.hpp"

/// <summary>
/// 最大递归深度
/// </summary>
/// <summary>
/// 最大弹射深度。超过后直接返回 0（不再注入背景能量）。
/// 注意：启用俄罗斯轮盘赌后，能走到上限的路径都带着补偿后的大权重，上限过低会造成可见的
/// 偏暗（对照实测：上限 8 时 256spp 均值偏低 0.34%，上限 16 时偏高 0.04% 而耗时几乎相同），
/// 因此这里取 16 作为"无偏差且仍有明显收益"的取值。
/// </summary>
constexpr int MAX_DEPTH = 16;

/// <summary>
/// 俄罗斯轮盘赌的起始弹射深度与最小存活概率：从该深度起按累计吞吐量决定是否继续追踪，
/// 中止时只保留本段的直接光，存活时按存活概率补偿权重（保持无偏）。前几段不赌，
/// 避免小权重路径过早截断导致方差爆炸。
/// </summary>
constexpr int RR_START_DEPTH = 3;
constexpr float RR_MIN_Q = 0.05f;


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
	/// <param name="throughput">到当前段为止累计的路径吞吐量（f*cos/pdf 连乘，含轮盘赌补偿），只用于轮盘赌决策</param>
	glm::vec3 trace(Ray ray, int depth, float bsdfPDF = 0.f, const glm::vec3 throughput = glm::vec3(1.f));
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