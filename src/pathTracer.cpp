#include "pathTracer.hpp"
#include "constant.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.hpp"
#include <thread>
namespace {
	/// <summary>
	/// ACES 电影级色调映射（Narkowicz 拟合）：把 HDR 亮度压到 [0,1]，高光平滑过渡不过曝
	/// </summary>
	glm::vec3 tonemapACES(const glm::vec3& x) {
		const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
		return glm::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.f, 1.f);
	}

	/// <summary>
	/// 线性颜色 -> sRGB 编码。8bit 图片必须做这一步，否则中间调整体偏暗
	/// </summary>
	glm::vec3 linearToSRGB(const glm::vec3& c) {
		glm::vec3 clamped = glm::max(c, glm::vec3(0));
		glm::vec3 lo = clamped * 12.92f;
		glm::vec3 hi = 1.055f * glm::pow(clamped, glm::vec3(1.f / 2.4f)) - 0.055f;
		return glm::mix(hi, lo, glm::lessThan(clamped, glm::vec3(0.0031308f)));
	}

	/// <summary>
	/// [0,1] 浮点 -> 8bit（四舍五入）
	/// </summary>
	unsigned char toByte(float v) {
		return static_cast<unsigned char>(glm::clamp(v, 0.f, 1.f) * 255.f + 0.5f);
	}
}


PathTracer::PathTracer(Scene* scene, Camera* camera) {
	m_scene = scene;
	m_camera = camera;
	m_sampler = new Sampler();
	m_spp = 16;
	int pixelNum = camera->getWidth() * camera->getHeight();
	m_image = new unsigned char[pixelNum * 4];
	//线性 HDR 累加缓冲，出图时再做曝光与色调映射
	m_hdrImage = new float[static_cast<size_t>(pixelNum) * 3]();
	m_exposure = camera->getExposure();
	for (int i = 0; i < pixelNum; i++) {
		int index = i * 4;
		m_image[index] = 0;
		m_image[index + 1] = 0;
		m_image[index + 2] = 0;
		m_image[index + 3] = 255;
	}
}

PathTracer::~PathTracer() {
	delete m_scene;
	delete m_camera;
	delete[] m_image;
	delete[] m_hdrImage;
	delete m_sampler;
}

void PathTracer::developImage() const {
	//把线性 HDR 缓冲"显影"成 8bit 显示图：曝光 -> ACES 色调映射 -> sRGB 编码
	const int pixelNum = m_camera->getWidth() * m_camera->getHeight();
	for (int i = 0; i < pixelNum; i++) {
		glm::vec3 hdr(m_hdrImage[i * 3], m_hdrImage[i * 3 + 1], m_hdrImage[i * 3 + 2]);
		//数值保护：NaN（0/0 之类）置 0，其余夹到 [0, 1e30]，避免写出未定义的颜色
		if (!(hdr[0] == hdr[0]) || !(hdr[1] == hdr[1]) || !(hdr[2] == hdr[2])) {
			hdr = glm::vec3(0);
		}
		hdr = glm::clamp(hdr, glm::vec3(0), glm::vec3(1e30f));
		glm::vec3 exposed = hdr * m_exposure;
		glm::vec3 mapped;
		switch (m_scene->getTonemap()) {
		case 0: mapped = glm::clamp(exposed, 0.f, 1.f); break;
		case 2: mapped = exposed / (1.f + exposed); break;
		default: mapped = tonemapACES(exposed); break;
		}
		mapped = linearToSRGB(mapped);
		const int index = i * 4;
		m_image[index] = toByte(mapped[0]);
		m_image[index + 1] = toByte(mapped[1]);
		m_image[index + 2] = toByte(mapped[2]);
		m_image[index + 3] = 255;
	}
}

void PathTracer::saveHDR(const std::string& modelName) const {
	//线性 HDR 结果（Radiance .hdr / RGBE），方便后期自己调曝光与色调映射
	std::string path = "results/" + modelName + "_" + std::to_string(m_spp) + ".hdr";
	stbi_flip_vertically_on_write(1);
	stbi_write_hdr(path.c_str(), m_camera->getWidth(), m_camera->getHeight(), 3, m_hdrImage);
}

void PathTracer::save(std::string modelName) const {
	stbi_flip_vertically_on_write(1);
	developImage();
	saveHDR(modelName);
	stbi_write_png(modelName.insert(0, "results/").append("_").append(std::to_string(m_spp)).append(".png").c_str(),
		m_camera->getWidth(), m_camera->getHeight(), 4, m_image, 0);
}

void PathTracer::render() {
	//将图像分为若干块，每块大小为32*32，然后将每一块的渲染分到不同的线程处理
	int width = static_cast<int>(glm::ceil(m_camera->getWidth() / 32.f));
	int height = static_cast<int>(glm::ceil(m_camera->getHeight() / 32.f));
	std::vector<std::unique_ptr<std::thread>> m_pixelThreads;
	std::function<void(PathTracer*, int, int)> f = &PathTracer::renderPixel;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			m_pixelThreads.push_back(std::make_unique<std::thread>(f, this, x, y));
		}
	}

	for (auto& thread : m_pixelThreads) {
		thread->join();
	}
}

void PathTracer::renderPixel(int x, int y) {
	//把图像分成 32x32 的小块，每个块由一个线程负责
	const int width = m_camera->getWidth();
	const int imageX0 = x << 5, imageX1 = glm::min((x + 1) << 5, width);
	const int imageY0 = y << 5, imageY1 = glm::min((y + 1) << 5, m_camera->getHeight());
	for (int n = imageY0; n < imageY1; n++) {
		for (int m = imageX0; m < imageX1; m++) {
			const int pixel = n * width + m;
			glm::vec3 color(0);
			m_sampler->initialize(static_cast<uint32_t>(pixel));
			for (int i = 0; i < m_spp; i++) {
				m_sampler->setIndex(i);
				glm::vec2 rnd = m_sampler->get2D(0);
				Ray ray = m_camera->generateRay(m + rnd.x, n + rnd.y);
				//线性 HDR 累加，不做逐采样截断：
				//逐采样 clamp 会把高光压暗并让均值有偏，正确的做法是累加辐亮度、出图时统一曝光 + 色调映射
				color += trace(ray, 0);
			}
			color /= static_cast<float>(m_spp);
			m_hdrImage[pixel * 3] = color[0];
			m_hdrImage[pixel * 3 + 1] = color[1];
			m_hdrImage[pixel * 3 + 2] = color[2];
		}
	}
}
glm::vec3 PathTracer::trace(Ray ray, int depth, const float bsdfPDF) {
	if (depth > MAX_DEPTH) {
		return m_scene->getBackground();
	}

	Intersection intersection;
	if (!m_scene->hit(ray, intersection)) {
		return m_scene->getBackground();
	}
	if (intersection.material->type == LIGHT) {
		//相机的第一条光线直接看到光源：光源采样策略无法产生该样本，MIS 权重为 1
		if (depth == 0) {
			return intersection.material->radiance;
		}
		//BSDF 采样策略命中光源：与光源采样策略做幂启发式 MIS 加权后计入
		float lightPDF = sampleLightPdf(ray, intersection);
		//单面光源背面不发光（光源采样策略同样会剔除），此时几何项使 PDF 为 0
		if (lightPDF <= 0.f) {
			return glm::vec3(0);
		}
		return intersection.material->radiance * powerHeuristic(bsdfPDF, lightPDF);
	}

	//单面材质：从背面射入的交点不参与散射。本工程场景由无厚度的墙面/挡板构成，
	//背面本就不该被照亮；若把法线翻到来向一侧，等于让光从背板漏进封闭空间
	//（bathroom2 中 Wood/Ceramic 等大面积挡板会因此整体偏亮 2.4~2.7 倍）。
	//两种采样策略都在这里返回，NEE 与 BSDF 采样对同一交点仍然一致。
	if (glm::dot(intersection.normal, ray.direction) > 0.f) {
		return glm::vec3(0);
	}
	m_sampler->shuffle();

	//δ 镜面的直接光贡献恒为 0（NEE 不可能命中），跳过采样以省去无效的光源采样与阴影射线
	glm::vec3 direct = intersection.material->isDeltaSpecular() ? glm::vec3(0) : sampleDirectLight(ray.direction, intersection);

	//计算间接光照，通过BRDF采样新方向
	glm::vec3 wi;
	float brdfPDF = 0.f;
	glm::vec3 brdf = intersection.brdf(ray.direction, wi, brdfPDF, m_sampler);
	if (brdfPDF < EPSILON) {
		return direct;
	}
	Ray newRay(intersection.point + intersection.normal * 1e-3f, wi);
	glm::vec3 radiance = trace(newRay, depth + 1, brdfPDF);
	glm::vec3 indirect = brdf * radiance * glm::dot(intersection.normal, wi) / brdfPDF;
	return direct + indirect;
}

glm::vec3 PathTracer::sampleDirectLight(glm::vec3 wo, Intersection& intersection) {
	if (m_scene->isLightEmpty()) {
		return glm::vec3(0);
	}
	auto lights = m_scene->getLights();
	glm::vec3 direct(0), brdf;
	int face;
	float cosLightTheta, cosTheta, lightPDF, brdfPDF, misWeight;
	Intersection shadowIntersection;
	//对场景中的所有光源进行采样，防止小面积光源难以被采到
	for (auto light : lights) {
		glm::vec3 lightPosition = light->sample(m_sampler->get1D(2), m_sampler->get2D(3), face);
		glm::vec3 radiance = light->getRadiance();
		//根据阴影光线方向调整原点向法线方向的偏移量
		float offsetScale = 1.f / glm::max(glm::dot(intersection.normal,
			glm::normalize(lightPosition - intersection.point)), 0.1f);
		glm::vec3 truePoint = intersection.point + intersection.normal * 1e-3f * offsetScale;
		glm::vec3 toLight = lightPosition - truePoint;

		Ray shadowRay(truePoint, glm::normalize(toLight));

		cosLightTheta = light->getCos(face, -shadowRay.direction);
		//面背向光源，无效采样
		if (cosLightTheta <= 0.f) {
			continue;
		}

		cosTheta = glm::dot(intersection.normal, shadowRay.direction);
		//此处应根据材质的透明度做进一步的处理，但本项目不处理透明材质，故阴影射线背向表面时也视作无效
		if (cosTheta <= 0.f) {
			continue;
		}
		//若阴影射线不符合BRDF分布也视为无效采样
		if (!intersection.brdf(wo, shadowRay.direction, brdf, brdfPDF, m_sampler)) {
			continue;
		}

		//阴影射线未击中物体或击中的物体不是所采样的光源面则视为无效采样
		//可见性判定用距离而不是"命中的面 id"：光源面与天花板/墙共面时，阴影射线在两者上的 t 完全相同，
		//返回哪一个取决于 BVH 叶内顺序（任意），会把大量本该有效的光源样本误判为遮挡而丢弃。
		//只要最近的遮挡物不比光面更近，就说明该着色点能看见光源。
		float lightDistance = glm::sqrt(glm::dot(toLight, toLight));
		if (!m_scene->hit(shadowRay, shadowIntersection) || shadowIntersection.t < lightDistance * (1.f - 1e-3f)) {
			continue;
		}
		
		lightPDF = glm::dot(toLight, toLight) / (light->getArea() * cosLightTheta);
		misWeight = powerHeuristic(lightPDF, brdfPDF);
		direct += misWeight * brdf * radiance * cosTheta / lightPDF;
	}
	
	return direct;
}

float PathTracer::sampleLightPdf(const Ray& ray, const Intersection& intersection) const {
	Light* light = m_scene->getLight(intersection.material->name);
	float cosLightTheta = light->getCos(intersection.id, -ray.direction);
	if (cosLightTheta <= 0.f) {
		return 0.f;
	}
	//与 sampleDirectLight 中同一套面积到立体角的变换
	glm::vec3 toLight = intersection.point - ray.origin;
	return glm::dot(toLight, toLight) / (light->getArea() * cosLightTheta);
}

float PathTracer::powerHeuristic(float pdf1, float pdf2) {
	if (glm::abs(pdf1) <= EPSILON && glm::abs(pdf2) <= EPSILON) {
		return 0.f;
	}
	float weight1 = pdf1 * pdf1;
	float weight2 = pdf2 * pdf2;
	return weight1 / (weight1 + weight2);
}
