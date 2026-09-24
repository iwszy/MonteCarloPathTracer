#include "model.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace {
	/// <summary>
	/// 判断行首关键字：需整体匹配且其后紧跟空白或行尾
	/// </summary>
	inline bool matchKeyword(const char* p, const char* end, const char* keyword) {
		while (*keyword != '\0') {
			if (p >= end || *p != *keyword) {
				return false;
			}
			++p;
			++keyword;
		}
		return p >= end || *p == ' ' || *p == '\t' || *p == '\r';
	}

	inline bool isBlank(char c) { return c == ' ' || c == '\t' || c == '\r'; }

	inline void skipBlank(const char*& p, const char* end) {
		while (p < end && isBlank(*p)) {
			++p;
		}
	}

	/// <summary>
	/// 解析浮点数：跳过前导空白、取最长合法前缀，结果与 iss >> float 逐位一致
	/// </summary>
	inline float readFloat(const char*& p) {
		char* stop = nullptr;
		float value = std::strtof(p, &stop);
		if (stop == p) {
			//非法输入：与流提取失败一致，返回 0 并跳过该 token，避免死循环
			while (*p != '\0' && !isBlank(*p)) { ++p; }
			return 0.f;
		}
		p = stop;
		return value;
	}

	/// <summary>
	/// 解析十进制整数
	/// </summary>
	inline int readInt(const char*& p) {
		char* stop = nullptr;
		long value = std::strtol(p, &stop, 10);
		if (stop == p) {
			while (*p != '\0' && !isBlank(*p)) { ++p; }
			return 0;
		}
		p = stop;
		return static_cast<int>(value);
	}

	/// <summary>
	/// 读取一个以空白结束的字符串 token
	/// </summary>
	inline std::string readToken(const char*& p, const char* end) {
		const char* begin = p;
		while (p < end && !isBlank(*p)) { ++p; }
		return std::string(begin, p - begin);
	}
}

Model::Model() {
	m_axisCenters = nullptr;
	m_axisMaximums = nullptr;
	m_axisMinimums = nullptr;
}

Model::~Model() {
}

void Model::loadMTL(const std::string& filepath) {
	std::ifstream file(filepath);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open mtl file " << filepath << "\n";
		return;
	}

	Material* currentMaterial = nullptr;
	std::string line;

	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') {
			continue;
		}
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "newmtl") {
			std::string name;
			iss >> name;
			m_materials[name] = Material();
			if (currentMaterial != nullptr) {
				currentMaterial->calculateType();
			}
			currentMaterial = &m_materials[name];
			currentMaterial->name = name;
		} else if (prefix == "Kd") {
			iss >> currentMaterial->diffuse.x >> currentMaterial->diffuse.y >> currentMaterial->diffuse.z;
		} else if (prefix == "Ks") {
			iss >> currentMaterial->specular.x >> currentMaterial->specular.y >> currentMaterial->specular.z;
		} else if (prefix == "Tr") {
			iss >> currentMaterial->transmission.x >> currentMaterial->transmission.y >> currentMaterial->transmission.z;
		} else if (prefix == "Ns") {
			iss >> currentMaterial->shininess;
		} else if (prefix == "Ni") {
			iss >> currentMaterial->ior;
		} else if (prefix == "map_Kd") {
			std::string mapName;
			iss >> mapName;
			size_t lastSlash = filepath.find_last_of("/\\");
			std::string baseFilepath = (lastSlash != std::string::npos) ?
				filepath.substr(0, lastSlash + 1) : "";
			currentMaterial->setMapKd(baseFilepath.append(mapName));
		}
	}
	currentMaterial->calculateType();
}

void Model::loadModel(std::string& filepath) {
	//一次性把整个文件读进内存，再自建扫描。原实现每行建一个输入流对象，每个面片顶点引用
	//还要再建一个并调用字符串转整数；bathroom2 有 124 万面、约 370 万个顶点引用，
	//光是建流与解析就占了加载时间的大头。
	std::ifstream file(filepath, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open obj file " << filepath << "\n";
	}
	m_modelName = filepath.substr(filepath.find_last_of("/\\") + 1);
	m_modelName = m_modelName.substr(0, m_modelName.find_last_of('.'));
	std::string content;
	file.seekg(0, std::ios::end);
	std::streamoff fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	if (fileSize > 0) {
		content.resize(static_cast<size_t>(fileSize));
		file.read(&content[0], fileSize);
		content.resize(static_cast<size_t>(file.gcount()));
	}

	//先数一遍各关键字数量，用于预留容量（避免上百万次扩容搬移）
	size_t vertexNum = 0, normalNum = 0, texcoordNum = 0, faceNum = 0;
	for (size_t pos = 0, size = content.size(); pos < size;) {
		size_t eol = content.find('\n', pos);
		if (eol == std::string::npos) { eol = size; }
		const char* begin = content.c_str() + pos;
		const char* end = content.c_str() + eol;
		pos = eol + 1;
		if (matchKeyword(begin, end, "vn")) { ++normalNum; }
		else if (matchKeyword(begin, end, "vt")) { ++texcoordNum; }
		else if (matchKeyword(begin, end, "v")) { ++vertexNum; }
		else if (matchKeyword(begin, end, "f")) { ++faceNum; }
	}

	std::string currentMaterial;
	Material* currentMat = &m_materials[currentMaterial];
	std::vector<glm::vec3> vertices;
	vertices.reserve(vertexNum);
	m_normals.reserve(normalNum);
	m_texcoords.reserve(texcoordNum);
	m_faces.reserve(faceNum);
	m_faceVertices.resize(static_cast<size_t>(faceNum) * 3);

	for (size_t pos = 0, size = content.size(); pos < size;) {
		size_t eol = content.find('\n', pos);
		if (eol == std::string::npos) { eol = size; }
		const char* p = content.c_str() + pos;
		const char* end = content.c_str() + eol;
		pos = eol + 1;
		skipBlank(p, end);
		if (p >= end || *p == '#') { continue; }

		if (matchKeyword(p, end, "v")) {
			//顶点坐标
			p += 1;
			glm::vec3 vertex;
			vertex.x = readFloat(p); vertex.y = readFloat(p); vertex.z = readFloat(p);
			vertices.push_back(vertex);
		}
		else if (matchKeyword(p, end, "vn")) {
			//法线
			p += 2;
			glm::vec3 normal;
			normal.x = readFloat(p); normal.y = readFloat(p); normal.z = readFloat(p);
			m_normals.push_back(normal);
		}
		else if (matchKeyword(p, end, "vt")) {
			//纹理坐标
			p += 2;
			glm::vec2 texcoord;
			texcoord.x = readFloat(p); texcoord.y = readFloat(p);
			texcoord = glm::clamp(texcoord, glm::vec2(0), glm::vec2(1));
			m_texcoords.push_back(texcoord);
		}
		else if (matchKeyword(p, end, "f")) {
			//面：顶点索引 / 纹理坐标索引 / 法线索引（本项目场景均为三角形、a/b/c 形式）
			p += 1;
			Face face;
			face.materialName = currentMaterial;
			const size_t vertexBase = m_faces.size() * 3;
			int vIndex = 0;
			while (vIndex < 3) {
				skipBlank(p, end);
				if (p >= end) { break; }
				int vertexIndex = readInt(p) - 1;
				int texcoordIndex = -1, normalIndex = -1;
				if (p < end && *p == '/') {
					++p;
					if (p < end && *p != '/') { texcoordIndex = readInt(p) - 1; }
					if (p < end && *p == '/') {
						++p;
						normalIndex = readInt(p) - 1;
					}
				}
				face.indices[vIndex] = glm::ivec2(texcoordIndex, normalIndex);
				m_faceVertices[vertexBase + vIndex] = (vertexIndex >= 0) ? vertices[vertexIndex] : glm::vec3(0);
				++vIndex;
			}
			m_faces.push_back(face);
			currentMat->faces.push_back(static_cast<int>(m_faces.size()) - 1);
		}
		else if (matchKeyword(p, end, "usemtl")) {
			//使用材质
			p += 6;
			skipBlank(p, end);
			currentMaterial = readToken(p, end);
			currentMat = &m_materials[currentMaterial];
		}
		else if (matchKeyword(p, end, "mtllib")) {
			//材质库文件
			p += 6;
			skipBlank(p, end);
			std::string mtlFilepath = readToken(p, end);
			size_t lastSlash = filepath.find_last_of("/\\");
			std::string baseFilepath = (lastSlash != std::string::npos) ? filepath.substr(0, lastSlash + 1) : "";
			loadMTL(baseFilepath + mtlFilepath);
		}
	}
}

const glm::vec3* Model::getFace(int i) const {
	return &m_faceVertices[static_cast<size_t>(i) * 3];
}
void Model::calAxisParams() {
	const size_t faceNum = m_faces.size();
	m_axisCenters = new glm::vec3[faceNum];
	m_axisMaximums = new glm::vec3[faceNum];
	m_axisMinimums = new glm::vec3[faceNum];
	glm::vec3 max, min;
	for (size_t i = 0; i < faceNum; i++) {
		const glm::vec3* v = &m_faceVertices[i * 3];
		for (int j = 0; j < 3; j++){
			max[j] = glm::max(v[0][j], glm::max(v[1][j], v[2][j]));
			min[j] = glm::min(v[0][j], glm::min(v[1][j], v[2][j]));
		}
		m_axisCenters[i] = glm::vec3((max + min) / 2.f);
		m_axisMaximums[i] = glm::vec3(max);
		m_axisMinimums[i] = glm::vec3(min);
	}
}

void Model::setLight(const std::string& materialName, const glm::vec3& radiance) {
	m_materials[materialName].type = LIGHT;
	m_materials[materialName].radiance = radiance;
	m_materials[materialName].faces.clear();
}

void Model::freeAxisParams() {
	delete[] m_axisCenters;
	delete[] m_axisMaximums;
	delete[] m_axisMinimums;
	for (auto& material : m_materials) {
		material.second.faces.clear();
	}
}