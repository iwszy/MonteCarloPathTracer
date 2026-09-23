#include "intersection.hpp"

#include "constant.hpp"

bool Intersection::brdf(const glm::vec3 wo, const glm::vec3 wi, glm::vec3& brdfVal, float& pdf, Sampler* sampler) {
	glm::vec3 localWi = m_transposeTransform * wi;
	if (material->type == DIFFUSE) {
		pdf = localWi.y * INV_PI;
		brdfVal = material->getDiffuse(uv) * INV_PI;
	}else if (material->type == SPECULAR) {
		//δ 镜面：NEE 命中它的概率为 0，直接返回零贡献（反射光全部由 BSDF 采样这一路带回）
		if (material->isDeltaSpecular()) {
			pdf = 0.f;
			brdfVal = glm::vec3(0);
			return false;
		}
		glm::vec3 h = glm::normalize(wi - wo);
		float hDotN = glm::dot(h, normal), oDotN = glm::dot(-wo, normal), iDotN = glm::dot(wi, normal);
		float d = ggx(hDotN), v = smithGGX(oDotN) * smithGGX(iDotN);
		//菲涅尔的自变量必须与 specularReflect 完全一致（半角向量余弦），否则两种采样策略
		//估计的是不同的 BRDF，且差异随颜色与角度变化，MIS 加权在数学上不成立
		glm::vec3 f = schlickFresnel(material->specular, glm::dot(wi, h));
		pdf = d * hDotN / (glm::dot(h, wi) * 4);
		brdfVal = d * f * v ;
	}else {
		brdfVal = evaluateMixed(wo, wi, pdf);
	}
	return pdf >= 0.f;
}

glm::vec3 Intersection::brdf(glm::vec3 wo, glm::vec3& wi, float& pdf, Sampler* sampler) {
	if (material->type == DIFFUSE) {
		return diffuseReflect(wi, pdf, sampler);
	}
	if (material->type == SPECULAR) {
		if (material->isDeltaSpecular()) {
			//理想 δ 镜面：反射方向唯一，不再对叶瓣做抖动采样
			glm::vec3 reflected = glm::reflect(wo, normal);
			float cosTheta = glm::dot(normal, reflected);
			if (cosTheta <= 0.f) {
				pdf = 0.f;
				return glm::vec3(0);
			}
			wi = glm::normalize(reflected);
			//δ 分布的密度不是普通函数：这里返回 (brdf, pdf) 的一组等价表示，
			//使上层 brdf * radiance * cos / pdf 恰好等于菲涅尔反射率 F 乘入射辐亮度，
			//同时把 pdf 交给 MIS 表示“镜面这一路远优于光源采样”。
			glm::vec3 f = schlickFresnel(material->specular, cosTheta);
			pdf = DELTA_SPECULAR_PDF;
			return f * DELTA_SPECULAR_PDF / cosTheta;
		}
		return specularReflect(wo, wi, pdf, sampler);
	}
	if (material->type == DIFFUSE_SPECULAR) {
		//按权重随机选择一个 lobe 采样出射方向（只是采样手段，不影响被估计的积分）
		float rnd = sampler->get1D(5);
		float lobePDF;
		if (rnd < material->diffuseRate) {
			diffuseReflect(wi, lobePDF, sampler);
		}else {
			specularReflect(wo, wi, lobePDF, sampler);
		}
		if (lobePDF <= 0.f) {
			//无效采样（例如反射方向落在表面以下）
			pdf = 0.f;
			return glm::vec3(0);
		}
		//再用完整 BRDF 与边缘密度求值，保证与光源采样路径估计同一个积分
		return evaluateMixed(wo, wi, pdf);
	}
	return glm::vec3(0);
}

glm::vec3 Intersection::diffuseReflect(glm::vec3& wi, float& pdf, Sampler* sampler) const {
	glm::vec2 rnd = sampler->get2D(6);
	float cosTheta = glm::sqrt(1 - rnd.x), sinTheta = glm::sqrt(rnd.x), phi = DOUBLE_PI * rnd.y;
	pdf = cosTheta * INV_PI;
	wi = m_transform * glm::vec3(sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi));
	return material->getDiffuse(uv) * INV_PI;
}

glm::vec3 Intersection::specularReflect(glm::vec3 wo, glm::vec3& wi, float& pdf, Sampler* sampler) {
	glm::vec2 rnd = sampler->get2D(6);
	float tmp = (1.f - rnd.x) / (material->a2 * rnd.x + 1.f - rnd.x);
	float cosTheta = glm::sqrt(tmp), sinTheta = glm::sqrt(1 - tmp), phi = DOUBLE_PI * rnd.y;
	glm::vec3 h = glm::vec3(sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi));
	h = m_transform * h;
	wi = glm::normalize(glm::reflect(wo, h));
	//反射方向落在表面以下时该采样无效：BRDF 为 0，返回 PDF=0 让调用方跳过该样本
	//（否则 dot(normal, wi) < 0 会产生负的间接光贡献，被 clamp 丢弃后表现为系统性偏亮）
	if (glm::dot(wi, normal) <= 0.f) {
		pdf = 0.f;
		return glm::vec3(0);
	}
	//微表面模型BRDF: fr = D * F * G 
	float hDotN = glm::dot(h, normal), oDotN = glm::dot(-wo, normal), iDotN = glm::dot(wi, normal);
	float d = ggx(hDotN), v = smithGGX(oDotN) * smithGGX(iDotN);
	//与 brdf() 求值路径保持一致：菲涅尔自变量取半角向量余弦
	glm::vec3 f = schlickFresnel(material->specular, glm::dot(wi, h));
	pdf = d * hDotN / (glm::dot(wi, h) * 4);
	return d * f * v;
}

glm::vec3 Intersection::evaluateMixed(glm::vec3 wo, glm::vec3 wi, float& pdf) const {
	glm::vec3 localWi = m_transposeTransform * wi;
	glm::vec3 diffuseVal = material->getDiffuse(uv) * INV_PI;
	glm::vec3 h = glm::normalize(wi - wo);
	float hDotN = glm::dot(h, normal), oDotN = glm::dot(-wo, normal), iDotN = glm::dot(wi, normal);
	float d = ggx(hDotN), v = smithGGX(oDotN) * smithGGX(iDotN);
	glm::vec3 f = schlickFresnel(material->specular, glm::dot(wi, h));
	float specularPDF = d * hDotN / (glm::dot(h, wi) * 4);
	//BRDF 为两项之和；PDF 为采样过程的边缘密度（= 两个 lobe 密度按采样权重求和）
	pdf = material->diffuseRate * localWi.y * INV_PI + (1 - material->diffuseRate) * specularPDF;
	//能量守恒的漫反射/镜面组合：只有没被镜面反射掉的那部分能量才进入漫反射层
	//（标准 PBR 写法 f = (1-F)·Kd/π + F·D·V）。原先直接相加 Kd/π + F·D·V，
	//在 Kd+Ks>1 的材质上每个反弹都放大能量，封闭场景里逐次累积会导致整幅图明显偏亮。
	return (glm::vec3(1) - f) * diffuseVal + d * f * v;
}

void Intersection::setNormal(glm::vec3 n) {
	normal = n;
	glm::vec3 tmp = (glm::abs(normal.x) > glm::abs(normal.y)) ? glm::vec3(normal.z, 0, -normal.x) :
		glm::vec3(0, -normal.z, normal.y);
	tmp = glm::normalize(tmp);
	glm::vec3 tBase = glm::normalize(glm::cross(normal, tmp));
	glm::vec3 b = glm::normalize(glm::cross(tBase, normal));
	m_transform = glm::mat3(tBase, normal, b);
	m_transposeTransform = glm::transpose(m_transform);
}

float Intersection::ggx(const float hDotN) const {
	return material->a2 * INV_PI / glm::pow(hDotN * hDotN * (material->a2 - 1) + 1, 2.f);
}

glm::vec3 Intersection::schlickFresnel(const glm::vec3 f0, float iDotN) const{
	float cosTheta = glm::clamp(1 - iDotN, 0.f, 1.f);
	return glm::mix(f0, glm::vec3(1), static_cast<float>(glm::pow(cosTheta, 5)));
}

float Intersection::smithGGX(float wDotN) const{
	return 1 / (wDotN + glm::sqrt(material->a2 + (1 - material->a2) * wDotN * wDotN));
}