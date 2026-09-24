#include "texture.hpp"
#include <iostream>
#include "stb/stb_image.hpp"

Texture::Texture(const std::string& filepath) {
	if (auto data = stbi_load(filepath.c_str(), &m_width, &m_height, &m_channels, 3); !data) {
        std::cerr << "Failed to load texture: " << filepath << "\n";
        m_width = m_height = 0;
        m_data = nullptr;
    }else {
        int pixelNum = m_width * m_height;
        m_data = new glm::vec3[pixelNum];
        //stb导入的像素值在0-255，且RGB值分别存储，为方便使用将每个像素的RGB值作为一个vec3存储，并将像素值归一到0-1
        for (int i = 0, index = 0; i < pixelNum; i++, index += 3) {
		//贴图文件按 sRGB 存储，这里解码到线性空间再参与渲染；
            //若不解码，贴图物体在输出端做 sRGB 编码后会明显偏亮（此前不解码也不编码，误差是相互抵消的）
            glm::vec3 srgb = glm::vec3(data[index], data[index + 1], data[index + 2]) / 255.f;
            glm::vec3 lo = srgb / 12.92f;
            glm::vec3 hi = glm::pow((srgb + 0.055f) / 1.055f, glm::vec3(2.4f));
            m_data[i] = glm::mix(hi, lo, glm::lessThan(srgb, glm::vec3(0.04045f)));
        }
        stbi_image_free(data);
    }
}

Texture::~Texture() {
	delete[] m_data;
}

glm::vec3 Texture::sample(const glm::vec2 uv) const {
	if (!m_data) {
        return glm::vec3(0);
	}
    //由于纹理坐标系的原点在图片左下角，而stb的坐标系原点在左上角，故y值需翻转
    float u = uv.x * m_width, v = (1 - uv.y) * m_height;
    int x = static_cast<int>(u), y = static_cast<int>(v);
    if (x == m_width) {
        x--;
    }
    if (y == m_height) {
        y--;
    }
    u -= x;
    v -= y;
    //获取在该点附近的4个像素点，然后通过双线性插值获取该点颜色
    int x1 = glm::clamp(x + 1, 0, m_width - 1), y1 = glm::clamp(y + 1, 0, m_height - 1);
    int index00 = y * m_width + x, index01 = y * m_width + x1, index10 = y1 * m_width + x, index11 = y1 * m_width + x1;
    return glm::mix(glm::mix(m_data[index00], m_data[index01], u), glm::mix(m_data[index10], m_data[index11], u), v);
}
