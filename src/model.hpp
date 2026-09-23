#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include "material.hpp"

/// <summary>
/// 网格面数据结构
/// </summary>
struct Face {
	/// <summary>
	/// 索引数组，每个元素都是一个向量，第一项表示uv坐标索引，第二项表示法线索引
	/// </summary>
	glm::ivec2* indices;
	/// <summary>
	///	对应的材质名称
	/// </summary>
	std::string materialName;

	Face() {
		indices = new glm::ivec2[3];
	}

	~Face() {
		delete[] indices;
	}

	Face(const Face& other) :
		indices(new glm::ivec2[3]),
		materialName(other.materialName)
	{
		std::copy_n(other.indices, 3, indices);
	}

	Face& operator=(const Face& other) {
		if (this != &other) {
			glm::ivec2* newIndices = new glm::ivec2[3];
			std::copy_n(other.indices, 3, newIndices);

			delete[] indices;

			indices = newIndices;
			materialName = other.materialName;
		}
		return *this;
	}
};

/// <summary>
/// 模型类，包含模型的导入操作以及存储模型的数据
/// </summary>
class Model {
public:
	Model();
	~Model();

	/// <summary>
	/// 导入mtl文件
	/// </summary>
	/// <param name="filepath">mtl文件的路径</param>
	void loadMTL(const std::string& filepath);
	/// <summary>
	/// 导入obj模型文件
	/// </summary>
	/// <param name="filepath">obj文件的路径</param>
	void loadModel(std::string& filepath);

	/// <summary>
	/// 根据面的索引获取对应的顶点
	/// </summary>
	/// <param name="i">面索引</param>
	/// <returns>对应三角面的顶点位置数组</returns>
	glm::vec3* getFace(int i) const;
	/// <summary>
	/// 根据面的索引获取对应的UV纹理坐标
	/// </summary>
	/// <param name="i">面索引</param>
	/// <returns>对应三角面的UV纹理坐标数组</returns>
	glm::vec2* getUV(int i) const;
	/// <summary>
	/// 根据面的索引获取对应的法线
	/// </summary>
	/// <param name="i">面索引</param>
	/// <returns>对应三角面的法线数组</returns>
	glm::vec3* getNormal(int i) const;
	/// <summary>
	/// 根据面的索引获取对应的材质
	/// </summary>
	/// <param name="i">面索引</param>
	/// <returns>对应三角面的材质</returns>
	const Material& getMaterial(int i) const { return m_materials.at(m_faces[i].materialName); }
	/// <summary>
	/// 根据材质名称获取对应的材质
	/// </summary>
	/// <param name="materialName">材质名称</param>
	/// <returns>对应的材质</returns>
	const Material& getMaterial(const std::string& materialName) const { return m_materials.at(materialName); }
	/// <summary>
	/// 将指定材质设置为光源类型
	/// </summary>
	/// <param name="materialName">材质名称</param>
	/// <param name="radiance">光源的radiance</param>
	void setLight(const std::string& materialName, const glm::vec3& radiance);
	/// <summary>
	/// 根据面的索引以及给定的轴获取对应三角形指定轴的中心位置
	/// </summary>
	/// <param name="i">面索引</param>
	/// <param name="axis">轴(0表示x轴，1表示y轴，2表示z轴)</param>
	/// <returns>指定三角形指定轴的中心位置</returns>
	float getAxisCenter(int i, int axis) const { return m_axisCenters[i][axis]; }
	/// <summary>
	/// 根据面的索引以及给定的轴获取对应三角形指定轴的位置的最大值
	/// </summary>
	/// <param name="i">面索引</param>
	/// <param name="axis">轴(0表示x轴，1表示y轴，2表示z轴)</param>
	/// <returns>指定三角形指定轴的位置的最大值</returns>
	float getAxisMaximum(int i, int axis) const { return m_axisMaximums[i][axis]; }
	/// <summary>
	/// 根据面的索引以及给定的轴获取对应三角形指定轴的位置的最小值
	/// </summary>
	/// <param name="i">面索引</param>
	/// <param name="axis">轴(0表示x轴，1表示y轴，2表示z轴)</param>
	/// <returns>指定三角形指定轴的位置的最小值</returns>
	float getAxisMinimum(int i, int axis) const { return m_axisMinimums[i][axis]; }
	/// <summary>
	/// 计算所有面的所有轴的中心位置、最大值、最小值
	/// </summary>
	void calAxisParams();
	/// <summary>
	/// 释放所存储的所有轴的中心位置、最大值、最小值的内存
	/// </summary>
	void freeAxisParams();

	/// <summary>
	/// 获取模型的面数量
	/// </summary>
	/// <returns>模型的面数量</returns>
	int getFaceNum() const { return static_cast<int>(m_faces.size()); }
	/// <summary>
	/// 获取模型名称
	/// </summary>
	/// <returns>模型名称</returns>
	std::string getModelName() const { return m_modelName; }
private:
	/// <summary>
	/// 模型的法线数组
	/// </summary>
	std::vector<glm::vec3> m_normals;
	/// <summary>
	/// 模型的UV坐标数组
	/// </summary>
	std::vector<glm::vec2> m_texcoords;
	/// <summary>
	/// 模型的面数组
	/// </summary>
	std::vector<Face> m_faces;
	/// <summary>
	/// 模型的各个面的顶点的数组
	/// </summary>
	std::vector<glm::vec3*> m_faceVertices;
	/// <summary>
	/// 模型的使用的材质库中材质名称到材质的映射
	/// </summary>
	std::unordered_map<std::string, Material> m_materials;
	/// <summary>
	/// 三角形各个轴的中心位置的数组
	/// </summary>
	glm::vec3* m_axisCenters;
	/// <summary>
	/// 三角形各个轴的位置的最大值的数组
	/// </summary>
	glm::vec3* m_axisMaximums;
	/// <summary>
	/// 三角形各个轴的位置的最小值的数组
	/// </summary>
	glm::vec3* m_axisMinimums;
	/// <summary>
	///	模型名称
	/// </summary>
	std::string m_modelName;
};