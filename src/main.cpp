#include <opencv2/opencv.hpp>

#include "../include/Trace.hpp"

using namespace cv;
using namespace sre;

int main() {
  char windName[] = "simple render engine";
  namedWindow(windName, 0);

  Tracer t(3, 5);
  // t.loadExampleScene();
  // t.load("../example/box/", "box.obj", "box.xml");
  // t.load("../example/veach-mis/", "veach-mis.obj", "veach-mis.xml");
  t.load("../example/cornell-box/", "cornell-box.obj", "cornell-box.xml");
  // t.load("../example/staircase/", "stairscase.obj", "staircase.xml");
  auto img = t.render();
  imshow(windName, img);

  if (waitKey(0) == static_cast<int>('s')) {
    imwrite("cornell-box.bmp", img);
  }
  destroyWindow(windName);
  return 0;
}