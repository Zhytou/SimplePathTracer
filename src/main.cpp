#include <format>
#include <iostream>

#include "Scene.hpp"
#include "Tracer.hpp"

using namespace spt;

int depth   = 4;
int rrdepth = 2;
int spp     = 16;
float rrp   = 0.8f;
float lum   = 5;
int cnt     = 20; // max bvh leaf node size
int ts      = 32; // tile size
int thd     = 20; // number of threads

std::filesystem::path config = "../ast/json/cuboid-sphere.json";

int main() {
    std::string name = std::format("{}_{}_{}_spp{}.png", config.stem().string(), depth, rrdepth, spp);

    try {
        std::cout << name << std::endl;
        Tracer tracer(depth, rrdepth, spp, rrp, lum, ts, thd);
        Scene scene(config, cnt);
        tracer.render(scene, name);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}