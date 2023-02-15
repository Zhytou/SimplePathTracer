#include "../include/Material.hpp"

int main() {
  char texName[3][100] = {"../example/staircase/textures/Tiles.jpg",
                          "../example/staircase/textures/Wallpaper.jpg",
                          "../example/staircase/textures/wood5.jpg"};
  sre::Material m;

  for (int i = 0; i < 3; i++) {
    m.setDiffuseTexture(texName[i]);
    auto color = m.getDiffusion(sre::Vec2<float>(0.5, 0.5));
    std::cout << color.x << '\t' << color.y << '\t' << color.z << std::endl;
  }
}