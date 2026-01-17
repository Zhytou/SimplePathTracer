#include <iostream>
#include <sstream>

#include "Trace.hpp"

using namespace spt;

int depth = 4;
int rrdepth = 2;
int spp = 4;

std::string dir = "../example/staircase/";
std::string config = "staircase.xml";

int main() {
  std::stringstream name;
  name << config.substr(0, config.size()-4) << '_' << "spp" << spp << ".png";

  Tracer tracer(depth, rrdepth, spp);
  tracer.load(dir, config, 40);
  tracer.render(name.str());

  return 0;
}