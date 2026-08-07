#ifndef SPT_IMAGE_HPP
#define SPT_IMAGE_HPP

#include <stb_image.h>
#include <stb_image_write.h>

#include "Utils.hpp"

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
    T getElement(int row, int col, int channel) const {
        if (row < 0 || row >= m_height || col < 0 || col >= m_width || channel < 0 || channel >= m_channels) {
            throw std::out_of_range("Image::getElement: out of range");
        }
        return m_data[row * m_width * m_channels + col * m_channels + channel];
    }
    template <size_t N>
    Vec<T, N> getPixel(int row, int col) const {
        Vec<T, N> pixel(static_cast<T>(0));
        for (int channel = 0; channel < std::min(int(N), m_channels); ++channel) {
            pixel[channel] = getElement(row, col, channel);
        }
        return pixel;
    }
    void setElement(int row, int col, int channel, T element) {
        if (row < 0 || row >= m_height || col < 0 || col >= m_width || channel < 0 || channel >= m_channels) {
            throw std::out_of_range("Image::setElement: out of range");
        }

        int index     = row * m_width * m_channels + col * m_channels + channel;
        m_data[index] = static_cast<T>(element);
    }
    template <size_t N>
    void setPixel(int row, int col, const Vec<T, N>& pixel) {
        for (int channel = 0; channel < std::min(int(N), m_channels); ++channel) {
            setElement(row, col, channel, pixel[channel]);
        }
    }

    /** 
     * @brief Read an image object from file.
     * @param path The image file path to load.
     * @param dchannels The desired number of channels for the image.
     * @param flip The indicator to flip the image vertically.
     * @return The created image.
     */
    static std::shared_ptr<Image<T>> read(const std::filesystem::path& path, int dchannels = 0, bool flip = false);
    /** 
     * @brief Write an image object to file.
     * @param image The image object to write.
     * @param path The image file path to write.
     */
    static void write(const std::shared_ptr<Image<T>>& image, const std::filesystem::path& path);

   private:
    T* m_data      = nullptr;
    int m_width    = 0;
    int m_height   = 0;
    int m_channels = 0;
};

template <typename T>
std::shared_ptr<Image<T>> Image<T>::read(const fs::path& path, int dchannels, bool flip) {
    // 0. Set flip mode
    stbi_set_flip_vertically_on_load(flip);

    // 1. Get image info
    int width    = 0;
    int height   = 0;
    int channels = 0;
    if (!stbi_info(path.c_str(), &width, &height, &channels)) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("Image::create: stbi load failed " + path.string() + " (" + (reason ? reason : "unknown") + ")");
    }

    // 2. Check image format
    bool is_hdr    = stbi_is_hdr(path.c_str());
    bool is_ushort = stbi_is_16_bit(path.c_str());
    bool is_uchar  = !is_hdr && !is_ushort;
    if constexpr (std::is_same_v<T, uint8_t>) {
        if (!is_uchar) {
            throw std::runtime_error("Image::read error: Image<uint8_t> can only load 8-bit images. File is HDR/16bit: " + path.string());
        }
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        if (!is_ushort) {
            throw std::runtime_error("Image::read error: Image<uint16_t> can only load 16-bit images. File is: " + path.string());
        }
    } else if constexpr (!std::is_same_v<T, float>) {
        throw std::runtime_error("Image::read error: Unsupported template type T."); // only uint8_t, uint16_t, and float are supported
    }

    // 3. Load image data based on given format and desired channels
    int n = dchannels == 0 ? width * height * channels : width * height * dchannels;
    std::vector<T> dst(n);
    if (is_uchar) {
        uint8_t* src = stbi_load(path.c_str(), &width, &height, &channels, dchannels);
        if (src == nullptr) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error("Image::create: stbi load failed " + path.string() + " (" + (reason ? reason : "unknown") + ")");
        }

        for (int i = 0; i < n; i++) {
            if constexpr (std::is_same_v<T, float>) { // normalize to [0, 1] if T is float
                dst[i] = static_cast<T>(src[i] / 255.0f);
            } else {
                dst[i] = static_cast<T>(src[i]);
            }
        }
        stbi_image_free(src);
    } else if (is_ushort) {
        uint16_t* src = stbi_load_16(path.c_str(), &width, &height, &channels, dchannels);
        if (src == nullptr) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error("Image::create: stbi load failed " + path.string() + " (" + (reason ? reason : "unknown") + ")");
        }

        for (int i = 0; i < n; i++) {
            if constexpr (std::is_same_v<T, float>) {
                dst[i] = static_cast<T>(src[i] / 65535.0f);
            } else {
                dst[i] = static_cast<T>(src[i]);
            }
        }
        stbi_image_free(src);
    } else {
        float* src = stbi_loadf(path.c_str(), &width, &height, &channels, dchannels);
        if (src == nullptr) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error("Image::create: stbi load failed " + path.string() + " (" + (reason ? reason : "unknown") + ")");
        }

        for (int i = 0; i < n; i++) {
            dst[i] = static_cast<T>(src[i]);
        }
        stbi_image_free(src);
    }

    // 4. Create image object
    auto image = std::make_shared<Image<T>>(width, height, dchannels == 0 ? channels : dchannels, const_cast<T*>(dst.data()));
    return image;
}

template <typename T>
void Image<T>::write(const std::shared_ptr<Image<T>>& image, const fs::path& path) {
    // 0. Check image object and storage path
    if (image == nullptr) {
        throw std::runtime_error("Image::write: image is nullptr");
    }
    if (fs::is_directory(path)) {
        throw std::runtime_error("Image::write: path is a directory");
    }

    // 1. Create parent directory if not exists
    fs::path parent_dir = path.parent_path();
    if (!parent_dir.empty() && !fs::exists(parent_dir)) {
        fs::create_directories(parent_dir);
    }

    // 2. Write image to file based on extension and data type
    int n    = image->m_width * image->m_height * image->m_channels;
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".hdr") {
        if constexpr (std::is_same_v<T, float>) {
            stbi_write_hdr(path.c_str(), image->m_width, image->m_height, image->m_channels, image->m_data);
        } else {
            std::vector<float> data(n);
            for (size_t i = 0; i < n; ++i) {
                if constexpr (std::is_same_v<T, uint8_t>) {
                    data[i] = static_cast<float>(image->m_data[i]) / 255.0f;
                } else if constexpr (std::is_same_v<T, uint16_t>) {
                    data[i] = static_cast<float>(image->m_data[i]) / 65535.0f;
                }
            }
            stbi_write_hdr(path.c_str(), image->m_width, image->m_height, image->m_channels, data.data());
        }
    } else if (ext == ".png") {
        if constexpr (std::is_same_v<T, uint8_t>) {
            stbi_write_png(path.c_str(), image->m_width, image->m_height, image->m_channels, image->m_data, 0);
        } else {
            std::vector<uint8_t> data(n);
            for (size_t i = 0; i < n; ++i) {
                if constexpr (std::is_same_v<T, uint16_t>) {
                    data[i] = static_cast<uint8_t>(image->m_data[i]);
                } else if constexpr (std::is_same_v<T, float>) {
                    data[i] = static_cast<uint8_t>(image->m_data[i] * 255.0f);
                }
            }
            stbi_write_png(path.c_str(), image->m_width, image->m_height, image->m_channels, data.data(), 0);
        }
    } else {
        throw std::runtime_error("Image::write error: Unsupported extension."); // only hdr and png are supported
    }
}

} // namespace spt

#endif
