#include "png_decoder.hpp"
#include "deflate.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>

uint32_t PNGDecoder::readBigEndian32(const uint8_t* data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

bool PNGDecoder::verifySignature(const std::vector<uint8_t>& data) {
    static const uint8_t signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (data.size() < 8) return false;
    return std::memcmp(data.data(), signature, 8) == 0;
}

PNGDecoder::PNGHeader PNGDecoder::parseIHDR(const std::vector<uint8_t>& data, size_t offset) {
    PNGHeader header;
    header.width = readBigEndian32(&data[offset]);
    header.height = readBigEndian32(&data[offset + 4]);
    header.bitDepth = data[offset + 8];
    header.colorType = data[offset + 9];
    header.compression = data[offset + 10];
    header.filter = data[offset + 11];
    header.interlace = data[offset + 12];
    return header;
}

std::vector<uint8_t> PNGDecoder::extractIDATData(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> idatData;
    size_t pos = 8; // Skip signature
    
    while (pos + 12 <= data.size()) {
        uint32_t length = readBigEndian32(&data[pos]);
        char type[5] = {0};
        std::memcpy(type, &data[pos + 4], 4);
        
        if (std::strcmp(type, "IDAT") == 0) {
            idatData.insert(idatData. end(), 
                           data. begin() + pos + 8, 
                           data.begin() + pos + 8 + length);
        } else if (std::strcmp(type, "IEND") == 0) {
            break;
        }
        
        pos += 12 + length; // length + type + data + crc
    }
    
    return idatData;
}

uint8_t PNGDecoder::paethPredictor(int a, int b, int c) {
    int p = a + b - c;
    int pa = std::abs(p - a);
    int pb = std::abs(p - b);
    int pc = std::abs(p - c);
    
    if (pa <= pb && pa <= pc) return static_cast<uint8_t>(a);
    if (pb <= pc) return static_cast<uint8_t>(b);
    return static_cast<uint8_t>(c);
}

void PNGDecoder::unfilterScanlines(std::vector<uint8_t>& rawData, uint32_t width,
                                    uint32_t height, int bytesPerPixel) {
    int stride = width * bytesPerPixel;
    std::vector<uint8_t> unfiltered(height * stride);
    std::vector<uint8_t> prevRow(stride, 0);
    
    size_t srcPos = 0;
    for (uint32_t y = 0; y < height; y++) {
        uint8_t filterType = rawData[srcPos++];
        
        for (int x = 0; x < stride; x++) {
            uint8_t raw = rawData[srcPos++];
            uint8_t a = (x >= bytesPerPixel) ? unfiltered[y * stride + x - bytesPerPixel] : 0;
            uint8_t b = prevRow[x];
            uint8_t c = (x >= bytesPerPixel) ? prevRow[x - bytesPerPixel] : 0;
            
            uint8_t result;
            switch (filterType) {
                case 0: result = raw; break;
                case 1: result = raw + a; break;
                case 2: result = raw + b; break;
                case 3: result = raw + ((a + b) / 2); break;
                case 4: result = raw + paethPredictor(a, b, c); break;
                default: throw std::runtime_error("Unknown filter type");
            }
            
            unfiltered[y * stride + x] = result;
        }
        
        std::copy(unfiltered.begin() + y * stride,
                 unfiltered.begin() + (y + 1) * stride,
                 prevRow.begin());
    }
    
    rawData = std::move(unfiltered);
}

Image PNGDecoder::decode(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    
    if (!verifySignature(data)) {
        throw std::runtime_error("Invalid PNG signature");
    }
    
    // Parse IHDR
    PNGHeader header = parseIHDR(data, 16); // 8 (sig) + 4 (len) + 4 (type)
    
    if (header.interlace != 0) {
        throw std::runtime_error("Interlaced PNGs not supported");
    }
    
    if (header.bitDepth != 8) {
        throw std::runtime_error("Only 8-bit depth supported");
    }
    
    int bytesPerPixel;
    switch (header.colorType) {
        case 0: bytesPerPixel = 1; break; // Grayscale
        case 2: bytesPerPixel = 3; break; // RGB
        case 4: bytesPerPixel = 2; break; // Grayscale + Alpha
        case 6: bytesPerPixel = 4; break; // RGBA
        default: throw std::runtime_error("Unsupported color type");
    }
    
    // Extract and decompress IDAT chunks
    std::vector<uint8_t> compressedData = extractIDATData(data);
    std::vector<uint8_t> rawData = Deflate::decompress(compressedData);
    
    // Unfilter
    unfilterScanlines(rawData, header.width, header.height, bytesPerPixel);
    
    // Convert to Image
    Image image(header.width, header.height);
    
    for (uint32_t y = 0; y < header.height; y++) {
        for (uint32_t x = 0; x < header.width; x++) {
            size_t pos = (y * header.width + x) * bytesPerPixel;
            Pixel& pixel = image.at(x, y);
            
            switch (header.colorType) {
                case 0: // Grayscale
                    pixel.r = pixel.g = pixel.b = rawData[pos];
                    break;
                case 2: // RGB
                    pixel. r = rawData[pos];
                    pixel.g = rawData[pos + 1];
                    pixel.b = rawData[pos + 2];
                    break;
                case 4: // Grayscale + Alpha (ignore alpha)
                    pixel.r = pixel.g = pixel.b = rawData[pos];
                    break;
                case 6: // RGBA (ignore alpha)
                    pixel.r = rawData[pos];
                    pixel.g = rawData[pos + 1];
                    pixel.b = rawData[pos + 2];
                    break;
            }
        }
    }
    
    return image;
}