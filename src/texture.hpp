#pragma once

#include <string>
#include <glm/glm.hpp>

/// <summary>
/// 纹理类
/// </summary>
class Texture {
public:
	Texture(const std::string& filepath);
	~Texture();

	/// <summary>
	/// 根据提供的纹理坐标获取该坐标处的像素值
	/// </summary>
	/// <param name="uv">纹理坐标</param>
	/// <returns>该坐标处的像素值</returns>
	glm::vec3 sample(glm::vec2 uv) const;
private:
	/// <summary>
	/// 纹理图片的宽度
	/// </summary>
	int m_width;
	/// <summary>
	/// 纹理图片的高度
	/// </summary>
	int m_height;
	/// <summary>
	/// 纹理图片的通道数
	/// </summary>
	int m_channels;
	/// <summary>
	/// 纹理图片的像素数组，每一个元素表示一个RGB值，RGB值已归一到0-1
	/// </summary>
	glm::vec3* m_data;
};