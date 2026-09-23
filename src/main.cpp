#include <chrono>
#include <iostream>
#include "pathTracer.hpp"

int main() {
    Model* model = new Model();
    //std::string modelPath = "models/veach-mis/veach-mis.obj", xmlPath = "models/veach-mis/veach-mis.xml";
    std::string modelPath = "models/cornell-box/cornell-box.obj", xmlPath = "models/cornell-box/cornell-box.xml";
    //std::string modelPath = "models/bathroom2/bathroom2.obj", xmlPath = "models/bathroom2/bathroom2.xml";
    Scene* scene = new Scene();

    auto start = std::chrono::steady_clock::now();
	model->loadModel(modelPath);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> loadObjTime = end - start;
    std::cout << "模型面数: " << model->getFaceNum() << ", 模型导入时间: " << loadObjTime.count() << "ms" << "\n";
    Camera* camera = scene->loadXML(xmlPath, model);
	if (camera == nullptr) {
        exit(1);
    }

    start = std::chrono::steady_clock::now();
    model->calAxisParams();
    scene->buildBVH(model);
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> buildBVHTime = end - start;
    std::cout << "BVH建立时间: " << buildBVHTime.count() << "ms" << "\n";

    model->freeAxisParams();
    PathTracer pathTracer(scene, camera);

    start = std::chrono::steady_clock::now();
    pathTracer.render();
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> renderTime = end - start;
    if (renderTime.count() > 100000) {
        std::chrono::duration<double> renderSecondTime = end - start;
        std::cout << "渲染时间: " << renderSecondTime.count() << "s" << "\n";
    }else {
    	std::cout << "渲染时间: " << renderTime.count() << "ms" << "\n";
    }

    //pathTracer.bilateralFilter(5, 12, 60);
    pathTracer.save(model->getModelName());
    delete model;
    return 0;
}