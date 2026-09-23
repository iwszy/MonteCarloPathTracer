#pragma once

#include "ray.hpp"
#include "constant.hpp"

/// <summary>
/// 包围盒类，使用的是轴对齐包围盒
/// </summary>
class BoundingBox {
public:
	/// <summary>
	/// 包围盒各轴上的最小值，依次为x轴，y轴，z轴
	/// </summary>
	float *min;
	/// <summary>
	/// 包围盒各轴上的最大值，依次为x轴，y轴，z轴
	/// </summary>
	float *max;

	BoundingBox() {
		min = new float[3];
		max = new float[3];
	}

	~BoundingBox() {
		delete[] min;
		delete[] max;
	}

	BoundingBox(float* min, float* max) {
		this->min = min;
		this->max = max;
	}

	BoundingBox(BoundingBox&& other) noexcept {
		min = other.min;
		max = other.max;
		other.min = nullptr;
		other.max = nullptr;
	}

	BoundingBox& operator=(BoundingBox&& other) noexcept {
		if (this != &other) {
			delete[] min;
			delete[] max;
			min = other.min;
			max = other.max;
			other.min = nullptr;
			other.max = nullptr;
		}
		return *this;
	}

	BoundingBox(const BoundingBox&) = delete;
	BoundingBox& operator=(const BoundingBox&) = delete;

	/// <summary>
	/// 判断射线是否与此包围盒有交点，若有，则存储时间只
	/// </summary>
	/// <param name="ray">需要求交的射线</param>
	/// <param name="t0">射线的起始时间值</param>
	/// <param name="t1">射线的终止时间值</param>
	/// <param name="t">射线进入此包围盒时的时间值，若无交点则无效</param>
	/// <returns>射线是否与包围盒有交点</returns>
	bool hit(Ray ray, const float t0, const float t1, float& t) const{
		float tEnter = t0, tExit = t1;
		for (int i = 0; i < 3; i++) {
			//若射线与轴平行，则判断射线原点在该轴上是否位于包围盒内，若在，则跳过该轴的后续判断，否则无交点
			if (glm::abs(ray.direction[i]) < EPSILON) {
				if (ray.origin[i] < min[i] && ray.origin[i] > max[i]) {
					return false;
				}
				continue;
			}
			//射线与轴求交
			float invDirection = 1 / ray.direction[i];
			float tMin = (min[i] - ray.origin[i]) * invDirection, tMax = (max[i] - ray.origin[i]) * invDirection;
			if (tMin > tMax) {
				std::swap(tMin, tMax);
			}
			tEnter = glm::max(tEnter, tMin);
			tExit = glm::min(tExit, tMax);
			//若进入时间大于离开时间或者离开时间小于起始时间，则无需做后续判断
			if (tEnter > tExit || tExit < t0) {
				return false;
			}
		}
		t = tEnter;
		return true;
	}
};