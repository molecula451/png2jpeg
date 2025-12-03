#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <cstdint>
#include <vector>

struct Pixel {
    uint8_t r, g, b;
    
    Pixel() : r(0), g(0), b(0) {}
    Pixel(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
};

class Image {
public:
    Image() : width_(0), height_(0) {}
    Image(uint32_t width, uint32_t height);
    
    void resize(uint32_t width, uint32_t height);
    
    Pixel& at(uint32_t x, uint32_t y);
    const Pixel& at(uint32_t x, uint32_t y) const;
    
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    
    const std::vector<Pixel>& pixels() const { return pixels_; }
    
private:
    uint32_t width_;
    uint32_t height_;
    std::vector<Pixel> pixels_;
};

#endif // IMAGE_HPP