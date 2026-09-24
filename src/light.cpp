#include "light.hpp"

Light::Light(const std::vector<int>& faces, const glm::vec3& radiance, Model* model) {
	m_model = model;
	m_radiance = radiance;
	m_area = 0;
	for (int face : faces) {
		m_faces.push_back(face);
	}
	calculateWeight();
}

float Light::getCos(int face, glm::vec3 direction) const {
	const glm::vec3* vertices = m_model->getFace(face);
	//直接通过叉乘三角形的两条边获取该面的法线
	glm::vec3 normal = glm::normalize(glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
	return glm::dot(normal, direction);
}

glm::vec3 Light::sample(float rnd, glm::vec2 uv, int& face) const {
	//此处通过重心坐标插值获得采样点，通过以下变换即可保证在三角形上均匀采样
	float u = 1 - glm::sqrt(uv.x), v = uv.y * (1 - u);
	face = selectFace(rnd);
	const glm::vec3* vertices = m_model->getFace(face);
	return vertices[0] * u + vertices[1] * v + (1 - u - v) * vertices[2];
}

int Light::selectFace(float rnd) const {
	//找到第一个不小于rnd的权重值，计算该值的索引，返回位于该索引的面
	auto it = std::lower_bound(m_faceWeights.begin(), m_faceWeights.end(), rnd);
	size_t faceIndex = std::distance(m_faceWeights.begin(), it);
	if (faceIndex == m_faces.size()) {
		faceIndex--;
	}
	return m_faces[faceIndex];
}

void Light::calculateWeight() {
	m_faceWeights.reserve(m_faces.size());
	std::vector<float> areas;
	for (const auto& face : m_faces) {
		const glm::vec3* vertices = m_model->getFace(face);
		//S(三角形) = 0.5 * bc * sinA = 0.5 * ||vector(b) × vector(c)||
		glm::vec3 cross = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
		float area = 0.5f * glm::length(cross);
		areas.push_back(area);
		m_area += area;
	}
	m_faceWeights.push_back(areas[0] / m_area);
	for (size_t i = 1; i < m_faces.size(); ++i) {
		m_faceWeights.push_back(areas[i] / m_area + m_faceWeights[i - 1]);
	}
}
