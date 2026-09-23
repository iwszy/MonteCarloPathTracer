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

Camera* Scene::loadXML(const std::string& filepath, Model* model) {
	XMLDocument doc;
	doc.LoadFile(filepath.c_str());
	XMLElement* root = doc.RootElement();
	XMLElement* cameraElement = root->FirstChildElement("camera");
	if (!cameraElement) {
		std::cerr << "Error: Not found camera config\n";
		return nullptr;
	}
	int width = cameraElement->IntAttribute("width");
	int height = cameraElement->IntAttribute("height");
	float fov = cameraElement->FloatAttribute("fovy");
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

	XMLElement* light = root->FirstChildElement("light");
	while (light != nullptr) {
		const char* materialName = light->Attribute("mtlname");
		const char* radianceStr = light->Attribute("radiance");
		glm::vec3 radiance;
		std::string token;
		int index = 0;
		std::istringstream iss(radianceStr);
		while (std::getline(iss, token, ',')) {
			radiance[index++] = std::stof(token);
		}
		Material lightMaterial = model->getMaterial(materialName);
		if (glm::dot(lightMaterial.diffuse, glm::vec3(1)) > EPSILON) {
			radiance *=lightMaterial.diffuse;
		}
		radiance /= 255.0;
		m_lights.push_back(new Light(lightMaterial.faces, radiance, model));
		m_lightMap[lightMaterial.name] = m_lights[m_lights.size() - 1];
		model->setLight(materialName, radiance);
		light = light->NextSiblingElement("light");
	}
	return new Camera(eye, lookAt, up, fov, width, height);
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