#pragma once

#include <memory>
#include <vector>
#include "model.hpp"
#include "boundingbox.hpp"
#include "intersection.hpp"

/// <summary>
/// BVH节点中最多可以包含的三角形数量
/// </summary>
constexpr int MAX_TRIANGLE = 10;

/// <summary>
/// BVH节点结构体
/// </summary>
struct BVHNode {
	/// <summary>
	/// 节点的包围盒（内联存储，遍历时无需指针跳转）
	/// </summary>
	BoundingBox bbox;
	/// <summary>
	/// 叶子节点所包含的三角形在 BVH 全局索引数组中的起始位置与数量；内部节点数量为 0。
	/// 三角形索引连续存放，省掉每个叶子各一个 vector 的堆分配。
	/// </summary>
	uint32_t triangleOffset = 0;
	uint32_t triangleCount = 0;
	/// <summary>
	/// 子节点索引
	/// </summary>
	uint32_t leftNode = 0;
	uint32_t rightNode = 0;
};

/// <summary>
/// BVH类，提供BVH的功能
/// </summary>
class BVH{
public:
	BVH(int* triangles, int n, Model* model);

	
	/// <summary>
	/// 计算射线与BVH的交点
	/// </summary>
	/// <param name="ray">要求交的射线</param>
	/// <param name="t0">射线的起始时间</param>
	/// <param name="t1">射线的终止时间</param>
	/// <param name="intersection">交点，若射线未击中BVH则无效</param>
	/// <param name="index">当前正在计算的BVH节点的索引</param>
	/// <returns>射线是否与BVH有交点</returns>
	bool hit(Ray& ray, float t0, float t1, Intersection &intersection, int index = 0);
private:
	/// <summary>
	/// BVH节点，以数组形式存储
	/// </summary>
	std::vector<BVHNode> m_nodes;
	/// BVH 全局三角形索引数组：各叶子节点的三角形按构建顺序连续存放
	std::vector<int> m_triangles;
	/// <summary>
	/// 模型类
	/// </summary>
	Model* m_model;

	/// <summary>
	/// 构建BVH节点
	/// </summary>
	/// <param name="triangles">模型面的索引数组</param>
	/// <param name="left">要处理的索引范围的左边界，此处为闭</param>
	/// <param name="right">要处理的索引范围的右边界，此处为闭</param>
	/// <param name="min">该节点的包围盒的各轴的最小值</param>
	/// <param name="max">该节点的包围盒的各轴的最大值</param>
	void build(int* triangles, int left, int right, float* min, float* max);
	/// <summary>
	/// 计算射线与三角面的交点
	/// </summary>
	/// <param name="ray">要求交的射线</param>
	/// <param name="t0">射线的起始时间</param>
	/// <param name="t1">射线的终止时间</param>
	/// <param name="intersection">交点，若射线未击中三角面则无效</param>
	/// <param name="id">三角面的索引</param>
	/// <returns>射线是否与三角面有交点</returns>
	bool hitTriangle(const Ray& ray, float t0, float t1, Intersection &intersection, int id) const;
	/// <summary>
	/// 计算包含指定范围的三角形的最小包围盒
	/// </summary>
	/// <param name="triangles">三角形索引数组</param>
	/// <param name="left">要处理的索引范围的左边界，此处为闭</param>
	/// <param name="right">要处理的索引范围的右边界，此处为闭</param>
	/// <param name="min">包围盒各轴的最小值</param>
	/// <param name="max">包围盒各轴的最大值</param>
	void calculateBoundingBox(int* triangles, int left, int right, float min[], float max[]) const;
	/// <summary>
	/// 计算包围盒的表面积
	/// </summary>
	/// <param name="min">包围盒各轴的最小值</param>
	/// <param name="max">包围盒各轴的最大值</param>
	/// <returns>包围盒的表面积</returns>
	float calculateSurface(float* min, float* max) const;
};