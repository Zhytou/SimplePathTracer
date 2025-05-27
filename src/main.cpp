#include <ctime>
#include <iostream>

#include "Trace.hpp"

using namespace spt;

int main() {
  srand(time(nullptr));
  char windName[] = "simple render engine";
  int depth = 3;
  int spp = 4;
  float threshold = 0.8;
  Tracer tracer(depth, spp, threshold);

  // tracer.load("../example/veach-mis/", {"veach-mis.obj"}, "veach-mis.xml");
  // tracer.load("../example/staircase/", {"stairscase.obj"}, "staircase.xml");
  // tracer.load("../example/cornell-box/", {"cornell-box.obj"}, "cornell-box.xml");
  // tracer.load("../example/box/", {"floor.obj", "light.obj", "left.obj", "right.obj", "shortbox.obj", "tallbox.obj"}, "cornell-box.xml");
  tracer.load("../example/metal-box/", {"floor.obj", "light.obj", "left.obj", "right.obj", "shortbox.obj", "tallbox.obj"}, "cornell-box.xml");

  // render
  time_t start = time(0);
  tracer.render();
  time_t end = time(0);
  std::cout << "Rendering time: " << difftime(end, start) << "s" << std::endl;

  return 0;
}