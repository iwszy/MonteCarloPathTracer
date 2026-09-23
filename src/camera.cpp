#include "camera.hpp"

Camera::Camera(glm::vec3 eye, glm::vec3 lookAt, glm::vec3 up, float fov, int width, int height, float exposure) {
	m_eye = eye;
	m_exposure = exposure;
	m_width = width;
	m_height = height;
	//相机看向非 z 轴时 |dz| 可能为 0，会让射线方向 normalize(0) 得到 NaN，
	//进而使 BVH 包围盒比较全部失效、遍历退化成指数级假死。这里改用真实距离，
	//且因为 m_top 与方向里的 m_distance 同时缩放，射线本身不会改变。
	m_distance = glm::length(lookAt - eye);
	if (m_distance < 1e-6f) {
		m_distance = 1.f;
	}
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
