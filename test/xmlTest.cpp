#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../src/Vec.hpp"

int main() {
  std::ifstream ifs;
  //"../example/cornell-box/cornell-box.xml"

  ifs.open("../example/veach-mis/veach-mis.xml", std::ios::in);

  if (!ifs.is_open()) {
    std::cout << "fail" << std::endl;
  }

  std::string buf;
  int i, j;

  int width, height;
  float fovy;
  float xyzs[3][3];
  std::unordered_map<std::string, Vec3<float>> lights;

  // xml
  getline(ifs, buf);
  // camera
  getline(ifs, buf);
  i = 0;
  while (i < buf.size()) {
    switch (buf[i]) {
      case 'w':
        assert(buf[i + 1] == 'i' && buf[i + 2] == 'd' && buf[i + 3] == 't' &&
               buf[i + 4] == 'h' && buf[i + 5] == '=' && buf[i + 6] == '\"');
        i += 7;
        j = buf.find('\"', i);
        width = stoi(buf.substr(i, j - i));
        i = j + 1;
        break;
      case 'h':
        assert(buf[i + 1] == 'e' && buf[i + 2] == 'i' && buf[i + 3] == 'g' &&
               buf[i + 4] == 'h' && buf[i + 5] == 't' && buf[i + 6] == '=' &&
               buf[i + 7] == '\"');
        i += 8;
        j = buf.find('\"', i);
        height = stoi(buf.substr(i, j - i));
        i = j + 1;
        break;
      case 'f':
        assert(buf[i + 1] == 'o' && buf[i + 2] == 'v' && buf[i + 3] == 'y' &&
               buf[i + 4] == '=' && buf[i + 5] == '\"');
        i += 6;
        j = buf.find('\"', i);
        fovy = stof(buf.substr(i, j - i));
        i = j + 1;
        break;
      default:
        i += 1;
        break;
    }
  }

  // eye-lookat-up
  for (int row = 0; row < 3; row++) {
    getline(ifs, buf);
    i = 0;

    int col = 0;
    while (i < buf.size()) {
      if (i > 0 && buf[i - 1] == ' ' &&
          (buf[i] == 'x' || buf[i] == 'y' || buf[i] == 'z')) {
        assert(buf[i + 1] == '=' && buf[i + 2] == '\"');
        i += 3;
        j = buf.find('\"', i);
        xyzs[row][col] = stof(buf.substr(i, j - i));
        i = j + 1;
        col += 1;
      } else {
        i += 1;
      }
    }
  }

  // lights
  while (getline(ifs, buf)) {
    std::string mtlname;
    Vec3<float> radiance;
    i = 0;
    while (i < buf.size()) {
      if (i > 0 && buf[i - 1] == ' ' && buf[i] == 'm') {
        assert(buf.substr(i, 9) == "mtlname=\"");
        i += 9;
        j = buf.find('\"', i);
        mtlname = buf.substr(i, j - i);
        i = j + 1;
      } else if (i > 0 && buf[i - 1] == ' ' && buf[i] == 'r') {
        assert(buf.substr(i, 10) == "radiance=\"");
        i += 10;
        j = buf.find(',', i);
        radiance.x = stof(buf.substr(i, j - i));
        i = j + 1;

        j = buf.find(',', i);
        radiance.y = stof(buf.substr(i, j - i));
        i = j + 1;

        j = buf.find('\"', i);
        radiance.z = stof(buf.substr(i, j - i));
        i = j + 1;
      } else {
        i += 1;
      }
    }
    if (!mtlname.empty()) {
      lights.emplace(mtlname, radiance);
    }
  }

  std::cout << "test: " << stof(std::string("   0.1")) << '\n';

  std::cout << "h&w: " << height << '\t' << width << '\n';
  std::cout << "fovy: " << fovy << '\n';
  std::cout << "eye: " << xyzs[0][0] << '\t' << xyzs[0][1] << '\t' << xyzs[0][2]
            << '\n';
  std::cout << "lookat: " << xyzs[1][0] << '\t' << xyzs[1][1] << '\t'
            << xyzs[1][2] << '\n';
  std::cout << "up: " << xyzs[2][0] << '\t' << xyzs[2][1] << '\t' << xyzs[2][2]
            << '\n';
  for (auto &[name, radiance] : lights) {
    std::cout << "mtlname: " << name << '\n'
              << "radiance: " << radiance.x << '\t' << radiance.y << '\t'
              << radiance.z << '\n';
  }
  std::cout << std::endl;
}