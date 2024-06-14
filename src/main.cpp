#include <opencv2/opencv.hpp>
#include <ctime>
#include <iostream>

#include "../include/Trace.hpp"

using namespace cv;
using namespace sre;

int main(int argc, char *argv[]) {
  char windName[] = "simple render engine";
  namedWindow(windName, 0);

  Tracer tracer(3, 20, -0.8);
  // tracer.loadExampleScene();
  tracer.load("../example/cornell-box/", "cornell-box.obj", "cornell-box.xml");
  // assert(argc == 4);
  // t.load(argv[1], argv[2], argv[3]);
  
  time_t start = time(0);
  auto img = tracer.render();
  time_t end = time(0);
  std::cout << "Rendering time: " << difftime(end, start) << "s" << std::endl;
  imshow(windName, img);

  while (1) {
    int key = waitKey(0);
    if (key == 'b') {
      break;
    } else if (key == 's') {
      cv::Mat img0;
      img.convertTo(img0, CV_8UC3, 255.0);
      imwrite("out.png", img0);
      break;
    }
  }

  destroyWindow(windName);
  return 0;
}