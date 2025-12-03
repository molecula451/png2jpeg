#ifndef JPEG_ENCODER_HPP
#define JPEG_ENCODER_HPP

#include "image.hpp"
#include <cstdint>
#include <vector>
#include <string>

class JPEGEncoder {
public:
    static void encode(const Image& image, const std::string& filename, int quality = 85);

private:
    class BitWriter {
    public:
        BitWriter(std::vector<uint8_t>& out) : output(out), buffer(0), bitCount(0) {}
        void writeBits(uint16_t bits, int count);
        void flush();
    private:
        std::vector<uint8_t>& output;
        uint32_t buffer;
        int bitCount;
    };

    static const int ZIGZAG[64];
    static int luminanceQuantTable[64];
    static int chrominanceQuantTable[64];

    static void rgbToYCbCr(uint8_t r, uint8_t g, uint8_t b,
                           float& y, float& cb, float& cr);
    static void forwardDCT(float block[8][8]);
    static void quantize(float block[8][8], const int quantTable[64], int output[64]);

    static void writeMarker(std::vector<uint8_t>& out, uint8_t marker);
    static void writeAPP0(std::vector<uint8_t>& out);
    static void writeDQT(std::vector<uint8_t>& out, const int table[64], int tableId);
    static void writeSOF0(std::vector<uint8_t>& out, uint32_t width, uint32_t height);
    static void writeDHT(std::vector<uint8_t>& out, const uint8_t* bits,
                         const uint8_t* values, int tcth, int count);
    static void writeSOS(std::vector<uint8_t>& out);

    static int getCategory(int value);
    static void generateHuffmanCodes(const uint8_t* bits, const uint8_t* values,
                                     uint16_t* codes, uint8_t* sizes);
    static void encodeBlock(BitWriter& writer, int block[64], int& prevDC,
                            const uint8_t* dcBits, const uint8_t* dcValues,
                            const uint8_t* acBits, const uint8_t* acValues);
};

#endif // JPEG_ENCODER_HPP
