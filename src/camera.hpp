#pragma once

#include <glm/glm.hpp>
#include "ray.hpp"
#include "constant.hpp"

/// <summary>
/// 相机类，包括相机的基础功能并提供生成射线的功能
/// </summary>
class Camera {
public:
	Camera(glm::vec3 eye, glm::vec3 lookAt, glm::vec3 up, float fov, int width, int height, float exposure = DEFAULT_EXPOSURE);

	/// <summary>
	/// 根据所给像素位置生成射线
	/// </summary>
	/// <param name="px">像素的x坐标</param>
	/// <param name="py">像素的y坐标</param>
	/// <returns>一条起点为相机位置，方向从起点到像素位置的射线</returns>
	Ray generateRay(float px, float py) const;

	/// <summary>
	/// 获取成像平面的宽度
	/// </summary>
	/// <returns>成像平面的宽度</returns>
	int getWidth() const { return m_width; }
	/// <summary>
	/// 获取成像平面的高度
	/// </summary>
	/// <returns>成像平面的高度</returns>
	int getHeight() const { return m_height; }
	/// <summary>
	/// 获取相机曝光系数（可由 xml 的 camera 元素 exposure 属性指定）
	/// </summary>
	/// <returns>曝光系数</returns>
	float getExposure() const { return m_exposure; }
private:

	/// <summary>
	/// 相机位置
	/// </summary>
	glm::vec3 m_eye;

	/// <summary>
	/// 曝光系数（由 xml 的 camera 元素指定，或取默认值）
	/// </summary>
	float m_exposure;
	/// <summary>
	/// 相机坐标系的基向量
	/// </summary>
	glm::vec3 m_u, m_v, m_w;
	/// <summary>
	/// 相机距离成像平面的位置
	/// </summary>
	float m_distance;
	/// <summary>
	/// 最终图像的宽度与高度
	/// </summary>
	int m_width, m_height;
	/// <summary>
	/// 成像平面的高度的一半
	/// </summary>
	float m_top;
	/// <summary>
	/// 成像平面的宽度的一半
	/// </summary>
	float m_right;
	/// <summary>
	/// 成像平面高度除以最终图像的高度的值
	/// </summary>
	float m_topDivHeightMul2;
};

