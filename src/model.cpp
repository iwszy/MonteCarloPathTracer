#include "model.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Model::Model() {
	m_axisCenters = nullptr;
	m_axisMaximums = nullptr;
	m_axisMinimums = nullptr;
}

Model::~Model() {
	for (auto vertices: m_faceVertices) {
		delete[] vertices;
	}
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
	std::ifstream file(filepath);
	if (!file.is_open()) {
		std::cerr << "Error: Could not open obj file " << filepath << "\n";
	}
	m_modelName = filepath.substr(filepath.find_last_of("/\\") + 1);
	m_modelName = m_modelName.substr(0, m_modelName.find_last_of('.'));
	std::string currentMaterial;
	std::string line;
	std::vector<glm::vec3> vertices;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') {
			continue;
		}
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;

		if (prefix == "v") {
			//顶点坐标
			glm::vec3 vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		} else if (prefix == "vn") {
			//法线
			glm::vec3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			m_normals.push_back(normal);
		} else if (prefix == "vt") {
			//纹理坐标
			glm::vec2 texcoord;
			iss >> texcoord.x >> texcoord.y;
			texcoord = glm::clamp(texcoord, glm::vec2(0), glm::vec2(1));
			m_texcoords.push_back(texcoord);
		} else if (prefix == "f") {
			//面
			Face face;
			face.materialName = currentMaterial;	
			std::string token;
			int vIndex = 0;
			glm::vec3* faceVertices = new glm::vec3[3];
			while (iss >> token) {
				//解析顶点索引格式：v/vt/vn
				glm::ivec3 vertex(-1);
				std::istringstream viss(token);
				std::string part;
				int idx = 0;
				while (std::getline(viss, part, '/')) {
					if (!part.empty()) {
						vertex[idx++] = std::stoi(part) - 1;
					}
				}
				face.indices[vIndex] = {vertex.y, vertex.z};
				faceVertices[vIndex++] = vertices[vertex.x];
			}
			
			m_faces.push_back(face);
			m_faceVertices.push_back(faceVertices);
			m_materials[currentMaterial].faces.push_back(static_cast<int>(m_faces.size()) - 1);
		} else if (prefix == "usemtl") {
			//使用材质
			iss >> currentMaterial;
		} else if (prefix == "mtllib") {
			//解析材质库文件
			std::string mtlFilepath;
			iss >> mtlFilepath;
			size_t lastSlash = filepath.find_last_of("/\\");
			std::string baseFilepath = (lastSlash != std::string::npos) ?
				filepath.substr(0, lastSlash + 1) : "";
			loadMTL(baseFilepath + mtlFilepath);
		}
	}
}

glm::vec3* Model::getFace(int i) const {
	return m_faceVertices[i];
}

glm::vec2* Model::getUV(int i) const {
	glm::vec2* faceUVs = new glm::vec2[3];
	faceUVs[0] = m_texcoords[m_faces[i].indices[0].x];
	faceUVs[1] = m_texcoords[m_faces[i].indices[1].x];
	faceUVs[2] = m_texcoords[m_faces[i].indices[2].x];
	return faceUVs;
}

glm::vec3* Model::getNormal(int i) const {
	glm::vec3* faceNormals = new glm::vec3[3];
	faceNormals[0] = m_normals[m_faces[i].indices[0].y];
	faceNormals[1] = m_normals[m_faces[i].indices[1].y];
	faceNormals[2] = m_normals[m_faces[i].indices[2].y];
	return faceNormals;
}

void Model::calAxisParams() {
	m_axisCenters = new glm::vec3[m_faceVertices.size()];
	m_axisMaximums = new glm::vec3[m_faceVertices.size()];
	m_axisMinimums = new glm::vec3[m_faceVertices.size()];
	glm::vec3 max, min;
	for (size_t i = 0; i < m_faceVertices.size(); i++) {
		for (int j = 0; j < 3; j++){
			max[j] = glm::max(m_faceVertices[i][0][j], glm::max(m_faceVertices[i][1][j], m_faceVertices[i][2][j]));
			min[j] = glm::min(m_faceVertices[i][0][j], glm::min(m_faceVertices[i][1][j], m_faceVertices[i][2][j]));
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