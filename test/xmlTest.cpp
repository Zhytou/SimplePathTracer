#include <iostream>
#include <string>

#include "../third-parties/pugixml/src/pugixml.hpp"
#include "../third-parties/tinyxml2/tinyxml2.h"

int main() {
  std::string configName = "test.xml";
  tinyxml2::XMLDocument tinyxml2doc;
  int errorId = 0;

  errorId = tinyxml2doc.LoadFile(configName.c_str());
  if (errorId != 0) {
    std::cout << "load " << configName
              << " xml file failed at error id:" << errorId << std::endl;
  }
  tinyxml2::XMLElement *root = tinyxml2doc.RootElement();

  configName = "../example/cornell-box/cornell-box.xml";
  pugi::xml_document pugixmldoc;
  if (pugixmldoc.load_file(configName.c_str()) == 0) {
    std::cout << "load " << configName << " xml file failed" << std::endl;
  };
}