#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include "pathTracer.hpp"

namespace {

/// <summary>
/// 命令行选项。取值优先级：命令行 > 场景 xml > 内置默认。
/// </summary>
struct Options {
    std::string scene = "cornell-box";
    std::string objPath;   ///< 非空则覆盖由 --scene 推导的 .obj
    std::string xmlPath;   ///< 非空则覆盖由 --scene 推导的 .xml
    int spp = 16;
    int width = 0;         ///< 0 表示沿用 xml 中的宽度
    int height = 0;        ///< 0 表示沿用 xml 中的高度
    float exposure = -1.f; ///< <=0 表示沿用 xml 中的曝光
    int tonemap = -1;      ///< <0 表示沿用 xml 中的色调曲线
    int maxDepth = -1;     ///< <0 表示用内置默认（MAX_DEPTH）
    bool rr = true;
    int threads = 0;       ///< 0 表示每个 tile 一个线程（默认）
    std::string outDir = "results";
    bool saveHDR = false;
    bool noSave = false;
    bool quiet = false;
};

void printUsage(const char* exe) {
    std::cout
        << "用法: " << exe << " [选项]" << "\n"
        << "\n"
        << "场景（默认 models/cornell-box/）" << "\n"
        << "  -s, --scene <名称>     使用 models/<名称>/<名称>.obj 与 .xml" << "\n"
        << "      --obj <路径>       直接指定 .obj（覆盖 --scene 推导）" << "\n"
        << "      --xml <路径>       直接指定 .xml（覆盖 --scene 推导）" << "\n"
        << "\n"
        << "渲染" << "\n"
        << "  -n, --spp <整数>       每像素采样数                        [默认 16]" << "\n"
        << "  -W, --width <整数>     输出宽度（覆盖 xml）                [默认取 xml]" << "\n"
        << "  -H, --height <整数>    输出高度（覆盖 xml）                [默认取 xml]" << "\n"
        << "      --exposure <浮点>  曝光系数（覆盖 xml）                [默认取 xml]" << "\n"
        << "      --tonemap <模式>   色调曲线: linear | aces | reinhard  [默认取 xml]" << "\n"
        << "      --max-depth <整数> 最大弹射深度                        [默认 16]" << "\n"
        << "      --rr <on|off>      俄罗斯轮盘赌                        [默认 on]" << "\n"
        << "      --threads <整数>   渲染线程数（0=每个 tile 一个线程）    [默认 0]" << "\n"
        << "\n"
        << "输出" << "\n"
        << "  -o, --out <目录>       结果目录（不存在会自动创建）        [默认 results]" << "\n"
        << "      --save-hdr         额外保存线性 HDR(.hdr)，默认关闭" << "\n"
        << "      --no-save          只渲染不写文件" << "\n"
        << "  -q, --quiet            只输出一行可脚本解析的统计" << "\n"
        << "\n"
        << "其它" << "\n"
        << "  -h, --help             显示本帮助" << "\n"
        << "\n"
        << "示例" << "\n"
        << "  " << exe << " -s bathroom2 -n 256" << "\n"
        << "  " << exe << " -s cornell-box -n 8 -W 320 -H 320 -q --no-save" << "\n";
}

[[noreturn]] void fail(const char* exe, const std::string& message) {
    std::cerr << "参数错误: " << message << "\n" << "\n";
    printUsage(exe);
    std::exit(2);
}

/// <summary>
/// 解析整数：要求整串都是数字，避免把 8abc 当成 8
/// </summary>
bool parseLong(const char* text, long& value) {
    if (text == nullptr || *text == 0) {
        return false;
    }
    char* stop = nullptr;
    long parsed = std::strtol(text, &stop, 10);
    if (stop == text || *stop != 0) {
        return false;
    }
    value = parsed;
    return true;
}

bool parseFloat(const char* text, float& value) {
    if (text == nullptr || *text == 0) {
        return false;
    }
    char* stop = nullptr;
    float parsed = std::strtof(text, &stop);
    if (stop == text || *stop != 0) {
        return false;
    }
    value = parsed;
    return true;
}

bool parseTonemap(const char* text, int& mode) {
    if (std::strcmp(text, "linear") == 0) { mode = 0; return true; }
    if (std::strcmp(text, "aces") == 0) { mode = 1; return true; }
    if (std::strcmp(text, "reinhard") == 0) { mode = 2; return true; }
    return false;
}

void parseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        std::string inlineValue;
        bool hasInlineValue = false;
        const std::size_t eq = arg.find('=');
        if (eq != std::string::npos) {
            inlineValue = arg.substr(eq + 1);
            arg = arg.substr(0, eq);
            hasInlineValue = true;
        }
        // 支持 "--spp 32" 与 "--spp=32" 两种写法
        auto valueOf = [&](const char* name) -> const char* {
            if (hasInlineValue) {
                return inlineValue.c_str();
            }
            if (i + 1 >= argc) {
                fail(argv[0], std::string(name) + " 需要一个取值");
            }
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (arg == "-s" || arg == "--scene") {
            opt.scene = valueOf("--scene");
        } else if (arg == "--obj") {
            opt.objPath = valueOf("--obj");
        } else if (arg == "--xml") {
            opt.xmlPath = valueOf("--xml");
        } else if (arg == "-n" || arg == "--spp") {
            long v = 0;
            if (!parseLong(valueOf("--spp"), v) || v < 1 || v > (1 << 20)) {
                fail(argv[0], "--spp 需要 1~1048576 之间的整数");
            }
            opt.spp = static_cast<int>(v);
        } else if (arg == "-W" || arg == "--width") {
            long v = 0;
            if (!parseLong(valueOf("--width"), v) || v < 1 || v > 65536) {
                fail(argv[0], "--width 需要 1~65536 之间的整数");
            }
            opt.width = static_cast<int>(v);
        } else if (arg == "-H" || arg == "--height") {
            long v = 0;
            if (!parseLong(valueOf("--height"), v) || v < 1 || v > 65536) {
                fail(argv[0], "--height 需要 1~65536 之间的整数");
            }
            opt.height = static_cast<int>(v);
        } else if (arg == "--exposure") {
            float v = 0.f;
            if (!parseFloat(valueOf("--exposure"), v) || !(v > 0.f)) {
                fail(argv[0], "--exposure 需要正数");
            }
            opt.exposure = v;
        } else if (arg == "--tonemap") {
            int mode = -1;
            if (!parseTonemap(valueOf("--tonemap"), mode)) {
                fail(argv[0], "--tonemap 只能是 linear / aces / reinhard");
            }
            opt.tonemap = mode;
        } else if (arg == "--max-depth") {
            long v = 0;
            if (!parseLong(valueOf("--max-depth"), v) || v < 1 || v > 4096) {
                fail(argv[0], "--max-depth 需要 1~4096 之间的整数");
            }
            opt.maxDepth = static_cast<int>(v);
        } else if (arg == "--rr") {
            const char* v = valueOf("--rr");
            if (std::strcmp(v, "on") == 0) {
                opt.rr = true;
            } else if (std::strcmp(v, "off") == 0) {
                opt.rr = false;
            } else {
                fail(argv[0], "--rr 只能是 on 或 off");
            }
        } else if (arg == "--threads") {
            long v = 0;
            if (!parseLong(valueOf("--threads"), v) || v < 0 || v > 65536) {
                fail(argv[0], "--threads 需要 0~65536 之间的整数（0 表示每个 tile 一个线程）");
            }
            opt.threads = static_cast<int>(v);
        } else if (arg == "-o" || arg == "--out") {
            opt.outDir = valueOf("--out");
            if (opt.outDir.empty()) {
                fail(argv[0], "--out 目录不能为空");
            }
        } else if (arg == "--save-hdr") {
            opt.saveHDR = true;
        } else if (arg == "--no-save") {
            opt.noSave = true;
        } else if (arg == "-q" || arg == "--quiet") {
            opt.quiet = true;
        } else {
            fail(argv[0], "未知参数 " + arg);
        }
    }

    if (opt.objPath.empty()) {
        opt.objPath = "models/" + opt.scene + "/" + opt.scene + ".obj";
    }
    if (opt.xmlPath.empty()) {
        opt.xmlPath = "models/" + opt.scene + "/" + opt.scene + ".xml";
    }
}

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    parseArgs(argc, argv, opt);

    std::error_code fsError;
    if (!std::filesystem::exists(opt.objPath, fsError)) {
        std::cerr << "找不到模型文件: " << opt.objPath << "\n";
        return 1;
    }
    if (!std::filesystem::exists(opt.xmlPath, fsError)) {
        std::cerr << "找不到场景文件: " << opt.xmlPath << "\n";
        return 1;
    }
    if (!opt.noSave) {
        std::filesystem::create_directories(opt.outDir, fsError);
        if (fsError) {
            std::cerr << "无法创建输出目录: " << opt.outDir << "\n";
            return 1;
        }
    }

    Model* model = new Model();
    Scene* scene = new Scene();

    auto start = std::chrono::steady_clock::now();
    model->loadModel(opt.objPath);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> loadObjTime = end - start;
    if (!opt.quiet) {
        std::cout << "模型面数: " << model->getFaceNum() << ", 模型导入时间: " << loadObjTime.count() << "ms" << "\n";
    }

    Camera* camera = scene->loadXML(opt.xmlPath, model, opt.width, opt.height);
    if (camera == nullptr) {
        std::cerr << "场景解析失败: " << opt.xmlPath << "\n";
        return 1;
    }

    start = std::chrono::steady_clock::now();
    model->calAxisParams();
    scene->buildBVH(model);
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> buildBVHTime = end - start;
    if (!opt.quiet) {
        std::cout << "BVH建立时间: " << buildBVHTime.count() << "ms" << "\n";
    }

    model->freeAxisParams();
    PathTracer pathTracer(scene, camera);
    //命令行渲染参数覆盖（未指定则沿用 xml / 内置默认）
    if (opt.tonemap >= 0) {
        scene->setTonemap(opt.tonemap);
    }
    if (opt.exposure > 0.f) {
        pathTracer.setExposure(opt.exposure);
    }
    pathTracer.setSpp(opt.spp);
    if (opt.maxDepth > 0) {
        pathTracer.setMaxDepth(opt.maxDepth);
    }
    pathTracer.setRussianRoulette(opt.rr);
    pathTracer.setThreadCount(opt.threads);
    pathTracer.setOutputDir(opt.outDir);
    pathTracer.setSaveHDR(opt.saveHDR);

    start = std::chrono::steady_clock::now();
    pathTracer.render();
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> renderTime = end - start;
    if (!opt.quiet) {
        if (renderTime.count() > 100000) {
            std::chrono::duration<double> renderSecondTime = end - start;
            std::cout << "渲染时间: " << renderSecondTime.count() << "s" << "\n";
        } else {
            std::cout << "渲染时间: " << renderTime.count() << "ms" << "\n";
        }
    }

    if (!opt.noSave) {
        pathTracer.save(model->getModelName());
    }
    if (opt.quiet) {
        //一行可脚本解析的统计，便于批量跑不同 spp 后直接汇总
        std::cout << "scene=" << opt.scene
                  << " faces=" << model->getFaceNum()
                  << " spp=" << opt.spp
                  << " size=" << camera->getWidth() << "x" << camera->getHeight()
                  << " load_ms=" << loadObjTime.count()
                  << " bvh_ms=" << buildBVHTime.count()
                  << " render_ms=" << renderTime.count()
                  << " png=" << (opt.noSave ? "-" : (opt.outDir + "/" + model->getModelName() + "_" + std::to_string(opt.spp) + ".png"))
                  << "\n";
    }

    delete model;
    return 0;
}
