#include <opencv2/opencv.hpp>
#include <ctime>
#include <iostream>

#include "../include/Trace.hpp"

using namespace cv;
using namespace sre;

int main() {
  char windName[] = "simple render engine";
  int depth = 3;
  int spp = 128;
  float threshold = 0.8;
  Tracer tracer(depth, spp, threshold);

  // tracer.load("../example/cornell-box/", {"cornell-box.obj"}, "cornell-box.xml");
  tracer.load("../example/simple cornell-box/", 
              {"floor.obj", "light.obj", "left.obj",
              "right.obj", "shortbox.obj", "tallbox.obj"},
              "cornell-box.xml");

  // render
  time_t start = time(0);
  auto img = tracer.render();
  time_t end = time(0);
  std::cout << "Rendering time: " << difftime(end, start) << "s" << std::endl;

  // show result
  namedWindow(windName, 0);
  imshow(windName, img);
  while (1) {
    int key = waitKey(0);
    if (key == 'b') {
      break;
    } else if (key == 's') {
      imwrite("out.png", img);
      break;
    }
  }
  destroyWindow(windName);
  return 0;
}