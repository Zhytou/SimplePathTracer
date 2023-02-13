#ifndef SRE_TEXTURE_HPP
#define SRE_TEXTURE_HPP

#include <opencv2/opencv.hpp>

#include <string>
#include <unordered_map>

namespace sre
{
    class Texture {
        private :
            cv::Mat image;
            static std::unordered_map<std::string, Texture*> textures;
        private :
            Texture() {}
            ~Texture() {}

        public:
            // getter.
            Vec3<float> getColorAt(const Vec2<float>& pos);
            
            static Texture* getInstance(const std::string& name) {
                if (textures.find(name) == textures.end()) {
                    Texture* ntex = new Texture();
                    ntex->textures = cv::read(name);
                }
                return textures[name];
            }
    };

    std::unordered_map<std::string, Texture*> Texture::textures;
} // namespace sre



#endif