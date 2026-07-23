#include <format>
#include <iostream>

#include "Scene.hpp"
#include "Tracer.hpp"

using namespace spt;

int depth   = 4;
int rrdepth = 2;
int spp     = 16;
float rrp   = 0.8f;
float lum   = 5.f;
int cnt     = 100; // bvh leaf node count

std::filesystem::path config = "../ast/json/dragon.json";

int main() {
    std::string name = std::format("{}_{}_{}_spp{}.png", config.stem().string(), depth, rrdepth, spp);

    try {
        Tracer tracer(depth, rrdepth, spp, rrp, lum);
        Scene scene(config, cnt);
        tracer.render(scene, name);
        std::cout << name << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}