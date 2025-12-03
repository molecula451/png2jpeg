#ifndef PNG_DECODER_HPP
#define PNG_DECODER_HPP

#include "image.hpp"
#include <string>
#include <vector>
#include <cstdint>

class PNGDecoder {
public:
    static Image decode(const std::string& filename);
    
private:
    struct PNGHeader {
        uint32_t width;
        uint32_t height;
        uint8_t bitDepth;
        uint8_t colorType;
        uint8_t compression;
        uint8_t filter;
        uint8_t interlace;
    };
    
    static uint32_t readBigEndian32(const uint8_t* data);
    static bool verifySignature(const std::vector<uint8_t>& data);
    static PNGHeader parseIHDR(const std::vector<uint8_t>& data, size_t offset);
    static std::vector<uint8_t> extractIDATData(const std::vector<uint8_t>& data);
    static void unfilterScanlines(std::vector<uint8_t>& rawData, uint32_t width, 
                                  uint32_t height, int bytesPerPixel);
    static uint8_t paethPredictor(int a, int b, int c);
};

#endif // PNG_DECODER_HPP