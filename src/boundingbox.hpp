#pragma once

#include <utility>
#include "ray.hpp"
#include "constant.hpp"

/// <summary>
/// 包围盒类，使用轴对齐包围盒。最小值/最大值改为内联数组存储，
/// 遍历时无需再经过 unique_ptr 与两个 float* 三重指针跳转。
/// </summary>
class BoundingBox {
public:
	float min[3];
	float max[3];

	BoundingBox() {
		min[0] = min[1] = min[2] = 0.f;
		max[0] = max[1] = max[2] = 0.f;
	}

	BoundingBox(const float* min, const float* max) { set(min, max); }

	/// <summary>
	/// 用两个长度为 3 的数组设置包围盒
	/// </summary>
	void set(const float* min, const float* max) {
		this->min[0] = min[0]; this->min[1] = min[1]; this->min[2] = min[2];
		this->max[0] = max[0]; this->max[1] = max[1]; this->max[2] = max[2];
	}

	/// <summary>
	/// 判断射线是否和该包围盒有交点，若有则存储进入时的时间值
	/// </summary>
	bool hit(const Ray& ray, const float t0, const float t1, float& t) const {
		float tEnter = t0, tExit = t1;
		for (int i = 0; i < 3; i++) {
			//射线方向与该轴平行时，只需判断起点在该轴上是否位于包围盒之外
			if (glm::abs(ray.direction[i]) < EPSILON) {
				if (ray.origin[i] < min[i] && ray.origin[i] > max[i]) {
					return false;
				}
				continue;
			}
			float invDirection = 1 / ray.direction[i];
			float tMin = (min[i] - ray.origin[i]) * invDirection, tMax = (max[i] - ray.origin[i]) * invDirection;
			if (tMin > tMax) {
				std::swap(tMin, tMax);
			}
			tEnter = glm::max(tEnter, tMin);
			tExit = glm::min(tExit, tMax);
			//进入时间大于离开时间、或离开时间小于起始时间，则不可能相交
			if (tEnter > tExit || tExit < t0) {
				return false;
			}
		}
		t = tEnter;
		return true;
	}
};