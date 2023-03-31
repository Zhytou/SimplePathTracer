#include <opencv2/opencv.hpp>

#include "../include/Trace.hpp"

using namespace cv;
using namespace sre;

int main(int argc, char *argv[]) {
  char windName[] = "simple render engine";
  namedWindow(windName, 0);

  Tracer t(3, 5);
  // t.loadExampleScene();
  // t.load("../example/box/", "box.obj", "box.xml");
  // t.load("../example/veach-mis/", "veach-mis.obj", "veach-mis.xml");
  // t.load("../example/cornell-box/", "cornell-box.obj", "cornell-box.xml");
  // t.load("../example/staircase/", "stairscase.obj", "staircase.xml");
  assert(argc == 4);
  t.load(argv[1], argv[2], argv[3]);
  auto img = t.render();
  imshow(windName, img);

  if (waitKey(0) == static_cast<int>('s')) {
    cv::Mat img0;
    img.convertTo(img0, CV_8UC3, 255.0);
    imwrite("staircase-new.png", img0);
  }
  destroyWindow(windName);
  return 0;
}