#include "image.hpp"
#include <stdexcept>

Image::Image(uint32_t width, uint32_t height) 
    : width_(width), height_(height), pixels_(width * height) {}

void Image::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    pixels_.resize(width * height);
}

Pixel& Image::at(uint32_t x, uint32_t y) {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("Pixel coordinates out of range");
    }
    return pixels_[y * width_ + x];
}

const Pixel& Image::at(uint32_t x, uint32_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("Pixel coordinates out of range");
    }
    return pixels_[y * width_ + x];
}