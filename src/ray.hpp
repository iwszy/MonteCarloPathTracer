#pragma once

#include <glm/glm.hpp>

/// <summary>
/// 射线类，提供射线的基本功能
/// </summary>
class Ray{
public:
	/// <summary>
	/// 射线的起点
	/// </summary>
	glm::vec3 origin;
	/// <summary>
	/// 射线的方向
	/// </summary>
	glm::vec3 direction;

	/// <summary>
	/// 根据射线的起点与方向构造射线
	/// </summary>
	/// <param name="o">射线起点</param>
	/// <param name="dir">射线方向</param>
	Ray(glm::vec3 o, glm::vec3 dir) {
		origin = o;
		direction = dir;
	}

	/// <summary>
	/// 根据给定的时间获取射线在该时间时的位置
	/// </summary>
	/// <param name="t">时间</param>
	/// <returns>射线在给定时间的位置</returns>
	glm::vec3 at(const float t) const {
		return origin + direction * t;
	}
};