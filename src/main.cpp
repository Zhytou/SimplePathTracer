#include <iostream>
#include <sstream>

#include "Trace.hpp"

using namespace spt;

int depth = 6;
int rrdepth = 3;
int spp = 32;

std::string dir = "../example/box/";
std::string config = "box.xml";

int main() {
  std::stringstream name;
  name << config.substr(0, config.size()-4) << '_' << "spp" << spp << ".png";

  Tracer tracer(depth, rrdepth, spp);
  tracer.load(dir, config);
  tracer.render(name.str());

  return 0;
}