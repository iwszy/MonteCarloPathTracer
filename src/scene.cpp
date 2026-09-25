#include "scene.hpp"
#include <iostream>
#include <sstream>
#include <xml/tinyxml2.h>

using namespace tinyxml2;

Scene::Scene() {
	m_bvh = nullptr;
	m_background = glm::vec3(0);
}

Scene::~Scene() {
	for (auto light : m_lights) {
		delete light;
	}
}

Camera* Scene::loadXML(const std::string& filepath, Model* model, int overrideWidth, int overrideHeight) {
	XMLDocument doc;
	if (doc.LoadFile(filepath.c_str()) != XML_SUCCESS) {
		std::cerr << "Error: Could not open xml file " << filepath << "\n";
		return nullptr;
	}
	XMLElement* root = doc.RootElement();
	if (root == nullptr) {
		std::cerr << "Error: xml file has no root element\n";
		return nullptr;
	}
	if (model->getFaceNum() == 0) {
		//模型为空通常意味着 obj 路径写错或文件打不开；继续执行会在取材质时抛异常
		std::cerr << "Error: model has no face, please check the obj path\n";
		return nullptr;
	}
	XMLElement* cameraElement = root->FirstChildElement("camera");
	if (!cameraElement) {
		std::cerr << "Error: Not found camera config\n";
		return nullptr;
	}
	int width = cameraElement->IntAttribute("width");
	int height = cameraElement->IntAttribute("height");
	//命令行 --width/--height 覆盖 xml 中的分辨率（用于快速预览与基准测试）
	if (overrideWidth > 0) { width = overrideWidth; }
	if (overrideHeight > 0) { height = overrideHeight; }
	float fov = cameraElement->FloatAttribute("fovy");
	//相机曝光：可选属性，缺省用默认值（不同场景参考图的曝光本来就不同，这里允许逐场景指定）
	float exposure = cameraElement->FloatAttribute("exposure", DEFAULT_EXPOSURE);
	//色调曲线：默认 ACES；作业参考图是线性截断管线，对应场景在 xml 里写 tonemap="linear"
	//mixed-material specular strength 0..1: F0 = mix(0.04, Ks, blend); 0.25 = compromise of the two references,
	//per-scene override via <camera specularBlend="..."> (bathroom2 uses 0).
	Intersection::setSpecularBlend(cameraElement->FloatAttribute("specularBlend", 0.25f));
	m_tonemap = 1;
	if (const char* tonemapStr = cameraElement->Attribute("tonemap")) {
		std::string tm(tonemapStr);
		if (tm == "linear") m_tonemap = 0;
		else if (tm == "reinhard") m_tonemap = 2;
	}

	glm::vec3 eye, lookAt, up;

	if (XMLElement* eyeElement = cameraElement->FirstChildElement("eye")) {
		eye.x = eyeElement->FloatAttribute("x");
		eye.y = eyeElement->FloatAttribute("y");
		eye.z = eyeElement->FloatAttribute("z");
	}

	if (XMLElement* lookAtElement = cameraElement->FirstChildElement("lookat")) {
		lookAt.x = lookAtElement->FloatAttribute("x");
		lookAt.y = lookAtElement->FloatAttribute("y");
		lookAt.z = lookAtElement->FloatAttribute("z");
	}

	if (XMLElement* upElement = cameraElement->FirstChildElement("up")) {
		up.x = upElement->FloatAttribute("x");
		up.y = upElement->FloatAttribute("y");
		up.z = upElement->FloatAttribute("z");
	}

	//背景(环境)辐射度：可选，缺省全黑。均匀环境是白炉/解析验证场景的前提。
	if (XMLElement* backgroundElement = root->FirstChildElement("background")) {
		const char* backgroundStr = backgroundElement->Attribute("radiance");
		if (backgroundStr != nullptr) {
			std::string token;
			int index = 0;
			std::istringstream iss(backgroundStr);
			while (index < 3 && std::getline(iss, token, ',')) {
				m_background[index++] = std::stof(token);
			}
		}
	}

	XMLElement* light = root->FirstChildElement("light");
	while (light != nullptr) {
		const char* materialName = light->Attribute("mtlname");
		const char* radianceStr = light->Attribute("radiance");
		if (materialName == nullptr || radianceStr == nullptr) {
			std::cerr << "Error: light element needs both mtlname and radiance\n";
			return nullptr;
		}
		glm::vec3 radiance;
		std::string token;
		int index = 0;
		std::istringstream iss(radianceStr);
		while (std::getline(iss, token, ',')) {
			radiance[index++] = std::stof(token);
		}
		if (!model->hasMaterial(materialName)) {
			std::cerr << "Error: light material not found in model: " << materialName << "\n";
			return nullptr;
		}
		const Material& lightMaterial = model->getMaterial(materialName);
		if (glm::dot(lightMaterial.diffuse, glm::vec3(1)) > EPSILON) {
			radiance *=lightMaterial.diffuse;
		}
		radiance /= 255.0;
		m_lights.push_back(new Light(lightMaterial.faces, radiance, model));
		m_lightMap[lightMaterial.name] = m_lights[m_lights.size() - 1];
		model->setLight(materialName, radiance);
		light = light->NextSiblingElement("light");
	}
	return new Camera(eye, lookAt, up, fov, width, height, exposure);
}

void Scene::buildBVH(Model* model) {
	int n = model->getFaceNum();
	int* triangles = new int[n];
	for (int i = 0; i < n; i++) {
		triangles[i] = i;
	}
	m_bvh = std::make_unique<BVH>(triangles, n, model);
	delete[] triangles;
}

bool Scene::hit(Ray ray, Intersection& intersection) const {
	return m_bvh->hit(ray, 0, std::numeric_limits<float>::max(), intersection);
}