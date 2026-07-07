#ifndef SPT_IMAGE_HPP
#define SPT_IMAGE_HPP

#include <filesystem>
#include <memory>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>
#include <vector>

namespace spt {

namespace fs = std::filesystem;

template <typename T>
class Image {
   public:
    Image(int width, int height, int channels, const T* data = nullptr)
        : m_width(width), m_height(height), m_channels(channels) {
        m_data = new T[width * height * channels];
        if (data != nullptr) {
            std::memcpy(m_data, data, width * height * channels * sizeof(T));
        } else {
            std::memset(m_data, 0, width * height * channels * sizeof(T));
        }
    }
    Image(const Image& other)            = delete;
    Image& operator=(const Image& other) = delete;
    Image(Image&& other) noexcept {
        m_data       = other.m_data;
        m_width      = other.m_width;
        m_height     = other.m_height;
        m_channels   = other.m_channels;
        other.m_data = nullptr;
    }
    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            delete[] m_data;
            m_data       = other.m_data;
            m_width      = other.m_width;
            m_height     = other.m_height;
            m_channels   = other.m_channels;
            other.m_data = nullptr;
        }
        return *this;
    }
    ~Image() {
        delete[] m_data;
    }

    T* getData() const { return m_data; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_channels; }

    void setElement(int row, int col, int channel, T element) {
        if (row < 0 || row >= m_height || col < 0 || col >= m_width || channel < 0 || channel >= m_channels) {
            throw std::out_of_range("Image::setElement: out of range");
        }

        int index     = row * m_width * m_channels + col * m_channels + channel;
        m_data[index] = element;
    }

    // Read an image object from file
    // @param path The image file path to load.
    // @param dchannels The desired number of channels for the image.
    // @param flip The indicator to flip the image vertically.
    // @return The created image.
    static std::shared_ptr<Image<T>> read(const std::filesystem::path& path, int dchannels = 0, bool flip = false);
    // Write an image object to file
    // @param image The image object to write.
    // @param path The image file path to write.
    static void write(const std::shared_ptr<Image<T>>& image, const std::filesystem::path& path);

   private:
    T* m_data      = nullptr;
    int m_width    = 0;
    int m_height   = 0;
    int m_channels = 0;
};

template <typename T>
std::shared_ptr<Image<T>> Image<T>::read(const fs::path& path, int dchannels, bool flip) {
    stbi_set_flip_vertically_on_load(flip);
    std::string filepath = fs::canonical(path).string();
    void* data           = nullptr;
    int width = 0, height = 0, channels = 0;

    if (!stbi_info(path.c_str(), &width, &height, &channels)) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("Image::create: stbi load failed " + path.string() + " (" + (reason ? reason : "unknown") + ")");
    }

    if (stbi_is_hdr(path.c_str())) {
        data = stbi_loadf(path.c_str(), &width, &height, &channels, dchannels);
    } else if (stbi_is_16_bit(path.c_str())) {
        data = stbi_load_16(path.c_str(), &width, &height, &channels, dchannels);
    } else {
        data = stbi_load(path.c_str(), &width, &height, &channels, dchannels);
    }

    if (data == nullptr) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("Image::create: stbi load failed " + path.string() + " (" + (reason ? reason : "unknown") + ")");
    }

    auto image = std::make_shared<Image<T>>(width, height, dchannels == 0 ? channels : dchannels, const_cast<T*>(data));
    if (data) {
        stbi_image_free(data);
    }

    return image;
}

template <typename T>
void Image<T>::write(const std::shared_ptr<Image<T>>& image, const fs::path& path) {
    if (image == nullptr) {
        throw std::runtime_error("Image::write: image is nullptr");
    }
    if (fs::is_directory(path)) {
        throw std::runtime_error("Image::write: path is a directory");
    }
    fs::path parent_dir = path.parent_path();
    if (!parent_dir.empty() && !fs::exists(parent_dir)) {
        fs::create_directories(parent_dir);
    }

    if (path.extension() != ".png") {
        throw std::runtime_error("Image::write: path extension must be png");
    } else {
        stbi_write_png(path.c_str(), image->m_width, image->m_height, image->m_channels, image->m_data, 0);
    }
}

} // namespace spt

#endif
