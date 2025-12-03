#include "jpeg_encoder.hpp"
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>

const int JPEGEncoder::ZIGZAG[64] = {
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63
};

int JPEGEncoder::luminanceQuantTable[64] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
};

int JPEGEncoder::chrominanceQuantTable[64] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

// Standard Huffman tables
static const uint8_t dcLuminanceBits[] = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t dcLuminanceValues[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static const uint8_t dcChrominanceBits[] = {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
static const uint8_t dcChrominanceValues[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static const uint8_t acLuminanceBits[] = {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125};
static const uint8_t acLuminanceValues[] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

static const uint8_t acChrominanceBits[] = {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119};
static const uint8_t acChrominanceValues[] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

void JPEGEncoder::BitWriter::writeBits(uint16_t bits, int count) {
    buffer = (buffer << count) | bits;
    bitCount += count;
    
    while (bitCount >= 8) {
        bitCount -= 8;
        uint8_t byte = (buffer >> bitCount) & 0xFF;
        output.push_back(byte);
        if (byte == 0xFF) {
            output.push_back(0x00); // Byte stuffing
        }
    }
}

void JPEGEncoder::BitWriter::flush() {
    if (bitCount > 0) {
        buffer <<= (8 - bitCount);
        uint8_t byte = buffer & 0xFF;
        output.push_back(byte);
        if (byte == 0xFF) {
            output.push_back(0x00);
        }
    }
}

void JPEGEncoder::rgbToYCbCr(uint8_t r, uint8_t g, uint8_t b,
                              float& y, float& cb, float& cr) {
    y  =  0.299f * r + 0.587f * g + 0.114f * b;
    cb = -0.168736f * r - 0.331264f * g + 0.5f * b + 128.0f;
    cr =  0.5f * r - 0.418688f * g - 0.081312f * b + 128.0f;
}

void JPEGEncoder::forwardDCT(float block[8][8]) {
    const float c1 = 0.980785f; // cos(pi/16)
    const float c2 = 0.923880f; // cos(2*pi/16)
    const float c3 = 0.831470f; // cos(3*pi/16)
    const float c4 = 0.707107f; // cos(4*pi/16) = 1/sqrt(2)
    const float c5 = 0.555570f; // cos(5*pi/16)
    const float c6 = 0.382683f; // cos(6*pi/16)
    const float c7 = 0.195090f; // cos(7*pi/16)
    
    // Slow but correct DCT implementation
    float temp[8][8];
    
    for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
            float sum = 0.0f;
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    sum += block[x][y] * 
                           std::cos((2.0f * x + 1.0f) * u * 3.14159265f / 16.0f) *
                           std::cos((2.0f * y + 1.0f) * v * 3.14159265f / 16.0f);
                }
            }
            float cu = (u == 0) ? 1.0f / std::sqrt(2.0f) : 1.0f;
            float cv = (v == 0) ? 1.0f / std::sqrt(2.0f) : 1.0f;
            temp[u][v] = 0.25f * cu * cv * sum;
        }
    }
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            block[i][j] = temp[i][j];
        }
    }
}

void JPEGEncoder::quantize(float block[8][8], const int quantTable[64], int output[64]) {
    for (int i = 0; i < 64; i++) {
        int x = ZIGZAG[i] / 8;
        int y = ZIGZAG[i] % 8;
        output[i] = static_cast<int>(std::round(block[x][y] / quantTable[ZIGZAG[i]]));
    }
}

void JPEGEncoder::writeMarker(std::vector<uint8_t>& out, uint8_t marker) {
    out.push_back(0xFF);
    out.push_back(marker);
}

void JPEGEncoder::writeAPP0(std::vector<uint8_t>& out) {
    writeMarker(out, 0xE0);
    // Length
    out.push_back(0x00);
    out.push_back(0x10);
    // JFIF identifier
    out.push_back('J');
    out.push_back('F');
    out.push_back('I');
    out.push_back('F');
    out.push_back(0x00);
    // Version
    out.push_back(0x01);
    out.push_back(0x01);
    // Units (0 = no units)
    out.push_back(0x00);
    // Density
    out.push_back(0x00);
    out.push_back(0x01);
    out.push_back(0x00);
    out.push_back(0x01);
    // Thumbnail size
    out.push_back(0x00);
    out.push_back(0x00);
}

void JPEGEncoder::writeDQT(std::vector<uint8_t>& out, const int table[64], int tableId) {
    writeMarker(out, 0xDB);
    // Length
    out.push_back(0x00);
    out.push_back(0x43);
    // Table info (4 bits precision, 4 bits table id)
    out.push_back(tableId);
    // Table values in zigzag order
    for (int i = 0; i < 64; i++) {
        out.push_back(static_cast<uint8_t>(table[i]));
    }
}

void JPEGEncoder::writeSOF0(std::vector<uint8_t>& out, uint32_t width, uint32_t height) {
    writeMarker(out, 0xC0);
    // Length
    out.push_back(0x00);
    out.push_back(0x11);
    // Precision
    out.push_back(0x08);
    // Height
    out.push_back((height >> 8) & 0xFF);
    out.push_back(height & 0xFF);
    // Width
    out.push_back((width >> 8) & 0xFF);
    out.push_back(width & 0xFF);
    // Number of components
    out.push_back(0x03);
    // Y component
    out.push_back(0x01); // ID
    out.push_back(0x11); // Sampling factor (1x1)
    out.push_back(0x00); // Quantization table ID
    // Cb component
    out.push_back(0x02);
    out.push_back(0x11);
    out.push_back(0x01);
    // Cr component
    out.push_back(0x03);
    out.push_back(0x11);
    out.push_back(0x01);
}

void JPEGEncoder::writeDHT(std::vector<uint8_t>& out, const uint8_t* bits, 
                            const uint8_t* values, int tcth, int count) {
    writeMarker(out, 0xC4);
    // Length
    int len = 19 + count;
    out.push_back((len >> 8) & 0xFF);
    out.push_back(len & 0xFF);
    // Table class and ID
    out.push_back(tcth);
    // Number of codes per length
    for (int i = 0; i < 16; i++) {
        out.push_back(bits[i]);
    }
    // Values
    for (int i = 0; i < count; i++) {
        out.push_back(values[i]);
    }
}

void JPEGEncoder::writeSOS(std::vector<uint8_t>& out) {
    writeMarker(out, 0xDA);
    // Length
    out.push_back(0x00);
    out.push_back(0x0C);
    // Number of components
    out.push_back(0x03);
    // Component 1 (Y)
    out.push_back(0x01);
    out.push_back(0x00); // DC/AC Huffman table
    // Component 2 (Cb)
    out.push_back(0x02);
    out.push_back(0x11);
    // Component 3 (Cr)
    out.push_back(0x03);
    out.push_back(0x11);
    // Spectral selection
    out.push_back(0x00);
    out.push_back(0x3F);
    out.push_back(0x00);
}

int JPEGEncoder::getCategory(int value) {
    if (value == 0) return 0;
    int absVal = std::abs(value);
    int cat = 0;
    while (absVal > 0) {
        absVal >>= 1;
        cat++;
    }
    return cat;
}

void JPEGEncoder::generateHuffmanCodes(const uint8_t* bits, const uint8_t* values,
                                        uint16_t* codes, uint8_t* sizes) {
    int k = 0;
    uint16_t code = 0;
    
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < bits[i]; j++) {
            sizes[values[k]] = i + 1;
            codes[values[k]] = code;
            code++;
            k++;
        }
        code <<= 1;
    }
}

void JPEGEncoder::encodeBlock(BitWriter& writer, int block[64], int& prevDC,
                               const uint8_t* dcBits, const uint8_t* dcValues,
                               const uint8_t* acBits, const uint8_t* acValues) {
    uint16_t dcCodes[256] = {0};
    uint8_t dcSizes[256] = {0};
    uint16_t acCodes[256] = {0};
    uint8_t acSizes[256] = {0};
    
    generateHuffmanCodes(dcBits, dcValues, dcCodes, dcSizes);
    generateHuffmanCodes(acBits, acValues, acCodes, acSizes);
    
    // Encode DC coefficient
    int dcDiff = block[0] - prevDC;
    prevDC = block[0];
    
    int dcCat = getCategory(dcDiff);
    writer.writeBits(dcCodes[dcCat], dcSizes[dcCat]);
    
    if (dcCat > 0) {
        int dcVal = dcDiff;
        if (dcDiff < 0) {
            dcVal = dcDiff - 1;
        }
        writer.writeBits(dcVal & ((1 << dcCat) - 1), dcCat);
    }
    
    // Encode AC coefficients
    int zeroCount = 0;
    for (int i = 1; i < 64; i++) {
        if (block[i] == 0) {
            zeroCount++;
        } else {
            while (zeroCount >= 16) {
                writer.writeBits(acCodes[0xF0], acSizes[0xF0]); // ZRL
                zeroCount -= 16;
            }
            
            int acCat = getCategory(block[i]);
            int symbol = (zeroCount << 4) | acCat;
            writer.writeBits(acCodes[symbol], acSizes[symbol]);
            
            int acVal = block[i];
            if (block[i] < 0) {
                acVal = block[i] - 1;
            }
            writer.writeBits(acVal & ((1 << acCat) - 1), acCat);
            
            zeroCount = 0;
        }
    }
    
    if (zeroCount > 0) {
        writer.writeBits(acCodes[0x00], acSizes[0x00]); // EOB
    }
}

void JPEGEncoder::encode(const Image& image, const std::string& filename, int quality) {
    // Adjust quantization tables based on quality
    int scaledLumQuant[64];
    int scaledChromQuant[64];
    
    int scale = (quality < 50) ? (5000 / quality) : (200 - quality * 2);
    
    for (int i = 0; i < 64; i++) {
        scaledLumQuant[i] = std::max(1, std::min(255, (luminanceQuantTable[i] * scale + 50) / 100));
        scaledChromQuant[i] = std::max(1, std::min(255, (chrominanceQuantTable[i] * scale + 50) / 100));
    }
    
    std::vector<uint8_t> output;
    
    // SOI marker
    writeMarker(output, 0xD8);
    
    // APP0 segment
    writeAPP0(output);
    
    // DQT segments
    writeDQT(output, scaledLumQuant, 0);
    writeDQT(output, scaledChromQuant, 1);
    
    // SOF0 segment
    writeSOF0(output, image.width(), image.height());
    
    // DHT segments
    writeDHT(output, dcLuminanceBits, dcLuminanceValues, 0x00, 12);
    writeDHT(output, acLuminanceBits, acLuminanceValues, 0x10, 162);
    writeDHT(output, dcChrominanceBits, dcChrominanceValues, 0x01, 12);
    writeDHT(output, acChrominanceBits, acChrominanceValues, 0x11, 162);
    
    // SOS segment
    writeSOS(output);
    
    // Encode image data
    BitWriter writer(output);
    
    int prevDCY = 0, prevDCCb = 0, prevDCCr = 0;
    
    uint32_t paddedWidth = ((image.width() + 7) / 8) * 8;
    uint32_t paddedHeight = ((image.height() + 7) / 8) * 8;
    
    for (uint32_t blockY = 0; blockY < paddedHeight; blockY += 8) {
        for (uint32_t blockX = 0; blockX < paddedWidth; blockX += 8) {
            float yBlock[8][8], cbBlock[8][8], crBlock[8][8];
            
            // Extract 8x8 block
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    uint32_t px = std::min(blockX + x, image.width() - 1);
                    uint32_t py = std::min(blockY + y, image.height() - 1);
                    
                    const Pixel& pixel = image.at(px, py);
                    float yVal, cbVal, crVal;
                    rgbToYCbCr(pixel.r, pixel.g, pixel.b, yVal, cbVal, crVal);
                    
                    yBlock[y][x] = yVal - 128.0f;
                    cbBlock[y][x] = cbVal - 128.0f;
                    crBlock[y][x] = crVal - 128.0f;
                }
            }
            
            // DCT
            forwardDCT(yBlock);
            forwardDCT(cbBlock);
            forwardDCT(crBlock);
            
            // Quantize
            int yQuant[64], cbQuant[64], crQuant[64];
            quantize(yBlock, scaledLumQuant, yQuant);
            quantize(cbBlock, scaledChromQuant, cbQuant);
            quantize(crBlock, scaledChromQuant, crQuant);
            
            // Encode
            encodeBlock(writer, yQuant, prevDCY, 
                       dcLuminanceBits, dcLuminanceValues,
                       acLuminanceBits, acLuminanceValues);
            encodeBlock(writer, cbQuant, prevDCCb,
                       dcChrominanceBits, dcChrominanceValues,
                       acChrominanceBits, acChrominanceValues);
            encodeBlock(writer, crQuant, prevDCCr,
                       dcChrominanceBits, dcChrominanceValues,
                       acChrominanceBits, acChrominanceValues);
        }
    }
    
    writer.flush();
    
    // EOI marker
    writeMarker(output, 0xD9);
    
    // Write to file
    std::ofstream file(filename, std::ios::binary);
    if (! file) {
        throw std::runtime_error("Cannot create output file: " + filename);
    }
    file.write(reinterpret_cast<const char*>(output.data()), output.size());
}