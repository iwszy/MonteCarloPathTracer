#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "model.hpp"

/// <summary>
/// 光源类，仅支持面光源
/// </summary>
class Light {
public:
    Light(const std::vector<int>& faces, const glm::vec3& radiance, Model* model);

    /// <summary>
    /// 计算指定方向与指定面的法线的夹角的余弦值
    /// </summary>
    /// <param name="face">面索引，此索引指的是该面在模型类中所有面中的索引</param>
    /// <param name="direction">要计算余弦值的方向</param>
    /// <returns>指定方向与指定面法线的夹角的余弦值</returns>
    float getCos(int face, glm::vec3 direction) const;
    /// <summary>
    /// 在光源表面随机采样一个点，不同面的采样概率=该面面积 / 光源总面积
    /// </summary>
    /// <param name="rnd">选择面的随机值</param>
    /// <param name="uv">在面上采样点的随机值</param>
    /// <param name="face">所选择的面</param>
    /// <returns>采样点位置</returns>
    glm::vec3 sample(float rnd, glm::vec2 uv, int& face) const;

    /// <summary>
    /// 获取光源总面积
    /// </summary>
    /// <returns>光源总面积</returns>
    float getArea() const { return m_area; }
    /// <summary>
    /// 获取光源的radiance
    /// </summary>
    /// <returns>光源的radiance</returns>
    glm::vec3 getRadiance() const { return m_radiance; }
private:
    /// <summary>
    /// 光源总面积
    /// </summary>
    float m_area;
    /// <summary>
    /// 光源radiance
    /// </summary>
    glm::vec3 m_radiance;
    /// <summary>
    /// 属于该光源的面的集合
    /// </summary>
    std::vector<int> m_faces;
    /// <summary>
    /// 该光源所有面的选择权重
    /// </summary>
    std::vector<float> m_faceWeights;
    /// <summary>
    /// 模型类
    /// </summary>
    Model* m_model;

    /// <summary>
    /// 根据随机值以及面的选择权重选择一个面
    /// </summary>
    /// <param name="rnd">0-1之间的随机值</param>
    /// <returns>所选择的面的索引，此索引指的是该面在模型类中所有面中的索引</returns>
    int selectFace(float rnd) const;
    /// <summary>
    /// 计算各面的选择概率
    /// </summary>
    void calculateWeight();
};
