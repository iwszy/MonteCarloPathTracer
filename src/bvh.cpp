#include "bvh.hpp"

#include <algorithm>
#include <limits>
#include <stack>

BVH::BVH(int* triangles, int n, Model* model) {
	m_model = model;
	//由于每个节点在构建前已经算好了包围盒，故第一个节点需在调用构建函数前单独计算
	float* min = new float[3], * max = new float[3];
	min[0] = std::numeric_limits<float>::max(); min[1] = min[0]; min[2] = min[0];
	max[0] = std::numeric_limits<float>::lowest(); max[1] = max[0]; max[2] = max[0];
	calculateBoundingBox(triangles, 0, n - 1, min, max);
	build(triangles, 0, n - 1, min, max);
}

void BVH::build(int* triangles, const int left, const int right, float* min, float* max) {
	int nodeIndex = m_nodes.size();
	m_nodes.emplace_back();
	m_nodes[nodeIndex].bbox = std::make_unique<BoundingBox>(min, max);
	if (right - left < MAX_TRIANGLE) {
		//节点包含的三角形数量不大于阈值则作为叶子节点
		m_nodes[nodeIndex].triangles.reserve(right - left + 1);
		for (int i = left; i <= right; i++) {
			m_nodes[nodeIndex].triangles.emplace_back(triangles[i]);
		}
	} else {
		//使用表面积启发式（SAH）构建BVH，判断10条划分轴
		float* leftMin = new float[3], * leftMax = new float[3], * rightMin = new float[3], * rightMax = new float[3];
		int mid = 0, finalDim = 0, isSame = 1;
		float minCost = std::numeric_limits<float>::max();
		for (int dim = 0; dim < 3; dim++) {
			auto maxID = std::max_element(triangles + left, triangles + right + 1, [&](int a, int b) {
				return m_model->getAxisCenter(a, dim) < m_model->getAxisCenter(b, dim);
				});
			auto minID = std::min_element(triangles + left, triangles + right + 1, [&](int a, int b) {
				return m_model->getAxisCenter(a, dim) < m_model->getAxisCenter(b, dim);
				});
			float step = (m_model->getAxisCenter(*maxID, dim) - m_model->getAxisCenter(*minID, dim)) / 11;
			float axis = m_model->getAxisCenter(*minID, dim) + step;
			if (glm::abs(step) > EPSILON) {
				isSame = 0;
			}else {
				//若在当前轴向上所有三角形的中心位置完全相同则跳过该轴判断
				//因为若进行后续判断只会得到其中1侧三角形数量为0的结果，此结果我们认定为一定不是最优结果
				//此外，若是承认此结果可能会导致1侧子节点与当前子节点处理相同的三角面从而无限递归
				continue;
			}
			for (int i = 0; i < 10; i++) {
				float tmpLeftMin[3], tmpLeftMax[3], tmpRightMin[3], tmpRightMax[3];
				auto id = std::partition(triangles + left, triangles + right + 1, [&](int a) {
					return m_model->getAxisCenter(a, dim) - axis <= EPSILON;
					});
				int index = static_cast<int>(std::distance(triangles + left, id)) - 1 + left;
				int leftNum = index - left + 1, rightNum = right - index;
				calculateBoundingBox(triangles, left, index, tmpLeftMin, tmpLeftMax);
				calculateBoundingBox(triangles, index + 1, right, tmpRightMin, tmpRightMax);
				float leftCost = leftNum == 0 ? 0 : calculateSurface(tmpLeftMin, tmpLeftMax) * leftNum;
				float rightCost = rightNum == 0 ? 0 : calculateSurface(tmpRightMin, tmpRightMax) * rightNum;
				float cost = leftCost + rightCost;
				if (cost < minCost) {
					leftMin[0] = tmpLeftMin[0]; leftMin[1] = tmpLeftMin[1]; leftMin[2] = tmpLeftMin[2];
					leftMax[0] = tmpLeftMax[0]; leftMax[1] = tmpLeftMax[1]; leftMax[2] = tmpLeftMax[2];
					rightMin[0] = tmpRightMin[0]; rightMin[1] = tmpRightMin[1]; rightMin[2] = tmpRightMin[2];
					rightMax[0] = tmpRightMax[0]; rightMax[1] = tmpRightMax[1]; rightMax[2] = tmpRightMax[2];
					minCost = cost;
					mid = index;
					finalDim = dim;
				}
				axis += step;
			}
		}
		if (isSame) {
			//若所有轴向上所有三角面的中心位置均相同，则会导致mid=0的情况
			//为避免此情况，此处强制进行均匀划分，并设置划分轴为x轴
			mid = (left + right) / 2;
			finalDim = 0;
			calculateBoundingBox(triangles, left, mid, leftMin, leftMax);
			calculateBoundingBox(triangles, mid + 1, right, rightMin, rightMax);
		}
		std::nth_element(triangles + left, triangles + mid, triangles + right + 1,
		[&](int a, int b) {
			return m_model->getAxisCenter(a, finalDim) < m_model->getAxisCenter(b, finalDim);
		});
		m_nodes[nodeIndex].leftNode = m_nodes.size();
		build(triangles, left, mid, leftMin, leftMax);
		m_nodes[nodeIndex].rightNode = m_nodes.size();
		build(triangles, mid + 1, right, rightMin, rightMax);
	}
}

bool BVH::hit(Ray& ray, const float t0, float t1, Intersection &intersection, const int index) {
	if (!m_nodes[index].triangles.empty()) {
		//叶子节点则逐一与包含的三角面求交
		bool isHit = false;
		for (int& triangle : m_nodes[index].triangles) {
			if (hitTriangle(ray, t0, t1, intersection, triangle)) {
				t1 = intersection.t;
				isHit = true;
			}
		}
		return isHit;
	}
	float leftT, rightT;
	int leftIndex = m_nodes[index].leftNode, rightIndex = m_nodes[index].rightNode;
	//首先对包围盒求交，若包围盒不交则无需进行后续判断
	bool isLeftHit = m_nodes[leftIndex].bbox->hit(ray, t0, t1, leftT), isRightHit = m_nodes[rightIndex].bbox->hit(ray, t0, t1, rightT);
	if (isLeftHit && isRightHit) {
		//若左右子节点均有交点，则根据相交时间的先后决定左右子节点的后续判断顺序
		//若第一个子节点有交点，则再次判断另一个子节点的包围盒在新的时间下是否有交，若有则进行后续判断
		if (leftT < rightT) {
			isLeftHit = hit(ray, t0, t1, intersection, leftIndex);
			if (isLeftHit) {
				t1 = intersection.t;
				if (m_nodes[rightIndex].bbox->hit(ray, t0, t1, rightT)) {
					isRightHit = hit(ray, t0, t1, intersection, rightIndex);
				}
			}else {
				isRightHit = hit(ray, t0, t1, intersection, rightIndex);
			}
		}else {
			isRightHit = hit(ray, t0, t1, intersection, rightIndex);
			if (isRightHit) {
				t1 = intersection.t;
				if (m_nodes[leftIndex].bbox->hit(ray, t0, t1, leftT)) {
					isLeftHit = hit(ray, t0, t1, intersection, leftIndex);
				}
			}else {
				isLeftHit = hit(ray, t0, t1, intersection, leftIndex);
			}
		}
		return isRightHit || isLeftHit;
	}
	if (isLeftHit) {
		return hit(ray, t0, t1, intersection, leftIndex);
	}
	if (isRightHit) {
		return hit(ray, t0, t1, intersection, rightIndex);
	}
	return false;
}

bool BVH::hitTriangle(const Ray& ray, const float t0, const float t1, Intersection& intersection, const int id) const {
	//根据射线与三角形求交的公式进行计算
	glm::vec3* face = m_model->getFace(id);
	glm::vec3 e1 = face[1] - face[0], e2 = face[2] - face[0], s = ray.origin - face[0];
	glm::vec3 s1 = glm::cross(ray.direction, e2), s2 = glm::cross(s, e1);
	float invSE1 = glm::dot(s1, e1);
	if (glm::abs(invSE1) < EPSILON) {
		//当三角形两个顶点位置相同时或射线与三角形完全平行时判定为没有交点
		return false;
	}
	invSE1 = 1 / invSE1;
	float t = glm::dot(s2, e2) * invSE1, beta = glm::dot(s1, s) * invSE1, gamma = glm::dot(s2, ray.direction) * invSE1;
	float alpha = 1 - beta - gamma;
	if (t < t0 || t > t1 || beta < 0 || gamma < 0 || alpha < 0) {
		return false;
	}
	intersection.point = ray.at(t);
	intersection.t = t;
	glm::vec3* normals = m_model->getNormal(id);
	glm::vec3 shadingNormal = glm::normalize(alpha * normals[0] + beta * normals[1] + gamma * normals[2]);
	//双面材质：把着色法线翻到光线射来的一侧。否则背面交点处光源采样会被 cos<=0 剔除，
	//而 BSDF 采样仍会向着"物体内部"的半球继续追踪，两种策略估计的积分不一致。
	if (glm::dot(shadingNormal, ray.direction) > 0.f) {
		shadingNormal = -shadingNormal;
	}
	intersection.setNormal(shadingNormal);
	intersection.material = &m_model->getMaterial(id);
	if (intersection.material->texture != nullptr) {
		glm::vec2* uvs = m_model->getUV(id);
		intersection.uv = alpha * uvs[0] + beta * uvs[1] + gamma * uvs[2];
		delete[] uvs;
	}
	intersection.id = id;
	delete[] normals;
	return true;
}

void BVH::calculateBoundingBox(int* triangles, int left, int right, float min[], float max[]) const {
	min[0] = std::numeric_limits<float>::max(); min[1] = min[0]; min[2] = min[0];
	max[0] = std::numeric_limits<float>::lowest(); max[1] = max[0]; max[2] = max[0];
	for (int i = left; i <= right; i++) {
		min[0] = glm::min(min[0], m_model->getAxisMinimum(triangles[i], 0));
		max[0] = glm::max(max[0], m_model->getAxisMaximum(triangles[i], 0));
		min[1] = glm::min(min[1], m_model->getAxisMinimum(triangles[i], 1));
		max[1] = glm::max(max[1], m_model->getAxisMaximum(triangles[i], 1));
		min[2] = glm::min(min[2], m_model->getAxisMinimum(triangles[i], 2));
		max[2] = glm::max(max[2], m_model->getAxisMaximum(triangles[i], 2));
	}
}

float BVH::calculateSurface(float* min, float* max) const {
	float a = max[0] - min[0], b = max[1] - min[1], c = max[2] - min[2];
	return 2 * (a * b + b * c + c * a);
}