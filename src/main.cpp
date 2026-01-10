#include <ctime>
#include <iostream>

#include "Trace.hpp"

using namespace spt;

int main() {
  int depth = 4;
  int spp = 8;
  float prob = 0.8;
  Tracer tracer(depth, spp, prob);

  // tracer.load("../example/veach-mis/", {"veach-mis.obj"}, "veach-mis.xml", 30);
  // tracer.load("../example/phong-veach-mis/", {"veach-mis.obj"}, "veach-mis.xml", 30);
  // tracer.load("../example/staircase/", {"stairscase.obj"}, "staircase.xml", 40);
  // tracer.load("../example/cornell-box/", {"cornell-box.obj"}, "cornell-box.xml", 50);
  tracer.load("../example/box/", {"floor.obj", "light.obj", "left.obj", "right.obj", "shortbox.obj", "tallbox.obj"}, "cornell-box.xml", 4);
  // tracer.load("../example/metal-box/", {"floor.obj", "light.obj", "left.obj", "right.obj", "shortbox.obj", "tallbox.obj"}, "cornell-box.xml", 4);
  // tracer.load("../example/glass-box/", {"floor.obj", "light.obj", "left.obj", "right.obj", "shortbox.obj", "tallbox.obj"}, "cornell-box.xml", 4);

  // render
  time_t start = time(0);
  tracer.render();
  time_t end = time(0);
  std::cout << "Time " << difftime(end, start) << "s" << std::endl;

  return 0;
}