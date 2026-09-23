#include "camera.hpp"

Camera::Camera(glm::vec3 eye, glm::vec3 lookAt, glm::vec3 up, float fov, int width, int height) {
	m_eye = eye;
	m_width = width;
	m_height = height;
	m_distance = glm::abs((lookAt - eye).z);
	//tan(fov/2) = t / |n|, r / t = width / height
	m_top = glm::tan(glm::radians(fov) / 2) * m_distance;
	m_topDivHeightMul2 = 2 * m_top / m_height;
	m_right = m_top * m_width / m_height;
	m_w = glm::normalize(eye - lookAt);
	m_u = glm::normalize(glm::cross(up, m_w));
	m_v = glm::normalize(glm::cross(m_w, m_u));
	m_distance = -m_distance;
}

Ray Camera::generateRay(const float px, const float py) const {
	//u = l + (r - l) * px / width = -r + 2 * r * px / width = 2 * width * t * px / width / height - r = (2 * t / height) * px - r
	float u = px * m_topDivHeightMul2 - m_right;
	//v = b + (t - b) * py / height = -t + 2 * t * py / height = (2 * t / height) * py - t
	float v = py * m_topDivHeightMul2 - m_top;
	return {m_eye, glm::normalize(u * m_u + v * m_v + m_distance * m_w)};
}
