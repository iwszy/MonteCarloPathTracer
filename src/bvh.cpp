#include "bvh.hpp"

#include <algorithm>
#include <limits>

BVH::BVH(int* triangles, int n, Model* model) {
	m_model = model;
	//由于每个节点在构建前已经算好了包围盒，故第一个节点需在调用构建函数前单独计算
	float min[3], max[3];
	min[0] = std::numeric_limits<float>::max(); min[1] = min[0]; min[2] = min[0];
	max[0] = std::numeric_limits<float>::lowest(); max[1] = max[0]; max[2] = max[0];
	m_triangles.reserve(n);
	calculateBoundingBox(triangles, 0, n - 1, min, max);
	build(triangles, 0, n - 1, min, max);
}

void BVH::build(int* triangles, const int left, const int right, float* min, float* max) {
	int nodeIndex = m_nodes.size();
	m_nodes.emplace_back();
	m_nodes[nodeIndex].bbox.set(min, max);
	if (right - left < MAX_TRIANGLE) {
		//节点包含的三角形数量不大于阈值则作为叶子节点
		m_nodes[nodeIndex].triangleOffset = static_cast<uint32_t>(m_triangles.size());
		for (int i = left; i <= right; i++) {
			m_triangles.emplace_back(triangles[i]);
		}
		m_nodes[nodeIndex].triangleCount = static_cast<uint32_t>(m_triangles.size()) - m_nodes[nodeIndex].triangleOffset;
	} else {
		//使用表面积启发式（SAH）构建BVH，判断10条划分轴
		//单遍分箱 SAH：与原先“每个候选平面各做一次 std::partition + 逐元素重算包围盒”数学等价
		//（同样的 11 个候选平面、同样的代价公式与比较顺序），但把每轴 10 次 O(n) 划分 + 30 次重算包围盒
		//换成 1 次 O(n) 归箱 + 每轴两次 O(11) 前后缀扫描，BVH 构建耗时约降一个数量级。
		constexpr int BIN_NUM = 11;
		float leftMin[3], leftMax[3], rightMin[3], rightMax[3];
		int mid = 0, finalDim = 0, isSame = 1;
		float minCost = std::numeric_limits<float>::max();
		float minCenter[3], maxCenter[3], step[3];
		for (int dim = 0; dim < 3; dim++) { minCenter[dim] = std::numeric_limits<float>::max(); maxCenter[dim] = std::numeric_limits<float>::lowest(); }
		//一趟扫描同时取三轴中心的最小/最大值（与 std::min_element / max_element 结果一致）
		for (int i = left; i <= right; i++) {
			int t = triangles[i];
			for (int dim = 0; dim < 3; dim++) {
				float c = m_model->getAxisCenter(t, dim);
				if (c < minCenter[dim]) minCenter[dim] = c;
				if (c > maxCenter[dim]) maxCenter[dim] = c;
			}
		}
		for (int dim = 0; dim < 3; dim++) {
			step[dim] = (maxCenter[dim] - minCenter[dim]) / BIN_NUM;
			if (glm::abs(step[dim]) > EPSILON) isSame = 0;
		}
		if (isSame) {
			//当前范围内所有三角形的中心位置完全相同：分箱无法分开（代价恒为 0，mid 会停在 0）
			//因此强制按中位数切分，并固定用 x 轴，避免无限递归
			mid = (left + right) / 2;
			finalDim = 0;
			calculateBoundingBox(triangles, left, mid, leftMin, leftMax);
			calculateBoundingBox(triangles, mid + 1, right, rightMin, rightMax);
		} else {
			float plane[3][BIN_NUM];
			for (int dim = 0; dim < 3; dim++) {
				plane[dim][0] = minCenter[dim];
				for (int k = 1; k < BIN_NUM; k++) {
					//候选平面按原先 axis += step 的方式逐步累加，连浮点累加误差一起复现，
					//使分箱边界与老实现逐位一致
					plane[dim][k] = plane[dim][k - 1] + step[dim];
				}
			}
			int binCount[3][BIN_NUM] = {};
			float binMin[3][BIN_NUM][3], binMax[3][BIN_NUM][3];
			for (int dim = 0; dim < 3; dim++)
				for (int b = 0; b < BIN_NUM; b++)
					for (int d2 = 0; d2 < 3; d2++) {
						binMin[dim][b][d2] = std::numeric_limits<float>::max();
						binMax[dim][b][d2] = std::numeric_limits<float>::lowest();
					}
			//一次归箱遍历：按三轴各自定位箱号，并累计该箱的包围盒与计数
			for (int i = left; i <= right; i++) {
				int t = triangles[i];
				for (int dim = 0; dim < 3; dim++) {
					if (glm::abs(step[dim]) <= EPSILON) continue;
					float c = m_model->getAxisCenter(t, dim);
					int bin = 0;
					for (int k = 1; k < BIN_NUM; k++) {
						if (c - plane[dim][k] > EPSILON) bin = k; else break;
					}
					binCount[dim][bin]++;
					for (int d2 = 0; d2 < 3; d2++) {
						binMin[dim][bin][d2] = glm::min(binMin[dim][bin][d2], m_model->getAxisMinimum(t, d2));
						binMax[dim][bin][d2] = glm::max(binMax[dim][bin][d2], m_model->getAxisMaximum(t, d2));
					}
				}
			}
			for (int dim = 0; dim < 3; dim++) {
				if (glm::abs(step[dim]) <= EPSILON) continue;
				//后缀：箱 i..10 的并集，即候选平面 i 右侧；前缀：箱 0..i-1，即左侧。
				//左右两侧的包围盒都是同一批面片包围盒的并集，因此与原实现逐位相同。
				float sufMin[BIN_NUM + 1][3], sufMax[BIN_NUM + 1][3];
				int sufNum[BIN_NUM + 1];
				for (int d2 = 0; d2 < 3; d2++) { sufMin[BIN_NUM][d2] = std::numeric_limits<float>::max(); sufMax[BIN_NUM][d2] = std::numeric_limits<float>::lowest(); }
				sufNum[BIN_NUM] = 0;
				for (int b = BIN_NUM - 1; b >= 0; b--) {
					sufNum[b] = sufNum[b + 1] + binCount[dim][b];
					for (int d2 = 0; d2 < 3; d2++) {
						sufMin[b][d2] = glm::min(sufMin[b + 1][d2], binMin[dim][b][d2]);
						sufMax[b][d2] = glm::max(sufMax[b + 1][d2], binMax[dim][b][d2]);
					}
				}
				float preMin[3], preMax[3];
				for (int d2 = 0; d2 < 3; d2++) { preMin[d2] = std::numeric_limits<float>::max(); preMax[d2] = std::numeric_limits<float>::lowest(); }
				int preNum = 0;
				for (int i = 1; i < BIN_NUM; i++) {
					preNum += binCount[dim][i - 1];
					for (int d2 = 0; d2 < 3; d2++) {
						preMin[d2] = glm::min(preMin[d2], binMin[dim][i - 1][d2]);
						preMax[d2] = glm::max(preMax[d2], binMax[dim][i - 1][d2]);
					}
					int rightNum = (right - left + 1) - preNum;
					float leftCost = preNum == 0 ? 0 : calculateSurface(preMin, preMax) * preNum;
					float rightCost = rightNum == 0 ? 0 : calculateSurface(sufMin[i], sufMax[i]) * rightNum;
					float cost = leftCost + rightCost;
					if (cost < minCost) {
						for (int d2 = 0; d2 < 3; d2++) {
							leftMin[d2] = preMin[d2]; leftMax[d2] = preMax[d2];
							rightMin[d2] = sufMin[i][d2]; rightMax[d2] = sufMax[i][d2];
						}
						minCost = cost;
						mid = left + preNum - 1;
						finalDim = dim;
					}
				}
			}
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
	if (m_nodes[index].triangleCount > 0) {
		//叶子节点：遍历其中的三角形，取最近命中
		bool isHit = false;
		const uint32_t begin = m_nodes[index].triangleOffset, end = begin + m_nodes[index].triangleCount;
		for (uint32_t i = begin; i < end; i++) {
			if (hitTriangle(ray, t0, t1, intersection, m_triangles[i])) {
				t1 = intersection.t;
				isHit = true;
			}
		}
		return isHit;
	}
	if (m_nodes[index].leftNode == 0 && m_nodes[index].rightNode == 0) {
		//退化产生的空叶子：既无三角形也无子节点，直接判为未命中
		//（旧实现把空的三角形列表当成内部节点，会递归回根节点，有无限递归风险）
		return false;
	}
	float leftT, rightT;
	int leftIndex = m_nodes[index].leftNode, rightIndex = m_nodes[index].rightNode;
	//首先对包围盒求交，若包围盒不交则无需进行后续判断
	bool isLeftHit = m_nodes[leftIndex].bbox.hit(ray, t0, t1, leftT), isRightHit = m_nodes[rightIndex].bbox.hit(ray, t0, t1, rightT);
	if (isLeftHit && isRightHit) {
		//若左右子节点均有交点，则根据相交时间的先后决定左右子节点的后续判断顺序
		//若第一个子节点有交点，则再次判断另一个子节点的包围盒在新的时间下是否有交，若有则进行后续判断
		if (leftT < rightT) {
			isLeftHit = hit(ray, t0, t1, intersection, leftIndex);
			if (isLeftHit) {
				t1 = intersection.t;
				if (m_nodes[rightIndex].bbox.hit(ray, t0, t1, rightT)) {
					isRightHit = hit(ray, t0, t1, intersection, rightIndex);
				}
			}else {
				isRightHit = hit(ray, t0, t1, intersection, rightIndex);
			}
		}else {
			isRightHit = hit(ray, t0, t1, intersection, rightIndex);
			if (isRightHit) {
				t1 = intersection.t;
				if (m_nodes[leftIndex].bbox.hit(ray, t0, t1, leftT)) {
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
	const glm::vec3* face = m_model->getFace(id);
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
	const Face& faceData = m_model->getFaceData(id);
	intersection.setNormal(glm::normalize(
		alpha * m_model->getVertexNormal(faceData.indices[0].y) +
		beta * m_model->getVertexNormal(faceData.indices[1].y) +
		gamma * m_model->getVertexNormal(faceData.indices[2].y)));
	intersection.material = &m_model->getMaterial(id);
	if (intersection.material->texture != nullptr) {
		intersection.uv =
			alpha * m_model->getVertexUV(faceData.indices[0].x) +
			beta * m_model->getVertexUV(faceData.indices[1].x) +
			gamma * m_model->getVertexUV(faceData.indices[2].x);
	}
	intersection.id = id;
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