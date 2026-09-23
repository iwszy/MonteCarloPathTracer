#include "intersection.hpp"

#include "constant.hpp"

bool Intersection::brdf(const glm::vec3 wo, const glm::vec3 wi, glm::vec3& brdfVal, float& pdf, Sampler* sampler) {
	glm::vec3 localWi = m_transposeTransform * wi;
	if (material->type == DIFFUSE) {
		pdf = localWi.y * INV_PI;
		brdfVal = material->getDiffuse(uv) * INV_PI;
	}else if (material->type == SPECULAR) {
		float hDotN = glm::dot(glm::normalize(wi - wo), normal), oDotN = glm::dot(-wo, normal), iDotN = glm::dot(wi, normal);
		float d = ggx(hDotN), v = smithGGX(oDotN) * smithGGX(iDotN);
		glm::vec3 f = schlickFresnel(material->specular, iDotN);
		pdf = d * hDotN / (glm::dot(glm::normalize(wi - wo), wi) * 4);
		brdfVal = d * f * v ;
	}else {
		float rnd = sampler->get1D(5);
		if (rnd < material->diffuseRate) {
			pdf = localWi.y * INV_PI * material->diffuseRate;
			brdfVal = material->getDiffuse(uv) * INV_PI;
		} else {
			float hDotN = glm::dot(glm::normalize(wi - wo), normal), oDotN = glm::dot(-wo, normal), iDotN = glm::dot(wi, normal);
			float d = ggx(hDotN), v = smithGGX(oDotN) * smithGGX(iDotN);
			glm::vec3 f = schlickFresnel(glm::mix(glm::vec3(0.04f), material->specular, 0.5), oDotN);
			pdf = d * hDotN / (glm::dot(glm::normalize(wi - wo), wi) * 4) * (1 - material->diffuseRate);
			brdfVal = d * f * v;
		}
	}
	return pdf >= 0.f;
}

glm::vec3 Intersection::brdf(glm::vec3 wo, glm::vec3& wi, float& pdf, Sampler* sampler) {
	if (material->type == DIFFUSE) {
		return diffuseReflect(wi, pdf, sampler);
	}
	if (material->type == SPECULAR) {
		return specularReflect(wo, wi, pdf, sampler);
	}
	if (material->type == DIFFUSE_SPECULAR) {
		float rnd = sampler->get1D(5);
		glm::vec3 color;
		//若既存在漫反射又存在镜面反射，则随机一个数根据其是否小于漫反射率决定进行漫反射还是镜面反射
		if (rnd < material->diffuseRate) {
			color = diffuseReflect(wi, pdf, sampler);
			pdf *= material->diffuseRate;
		}else {
			color = specularReflect(wo, wi, pdf, sampler);
			pdf *= (1 - material->diffuseRate);
		}
		return color;
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
	//微表面模型BRDF: fr = D * F * G 
	float hDotN = glm::dot(h, normal), oDotN = glm::dot(-wo, normal), iDotN = glm::dot(wi, normal);
	float d = ggx(hDotN), v = smithGGX(oDotN) * smithGGX(iDotN);
	glm::vec3 f = schlickFresnel(material->specular, iDotN);
	pdf = d * hDotN / (glm::dot(wi, h) * 4);
	return d * f * v;
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