#include "deflate.hpp"
#include <stdexcept>
#include <algorithm>

uint32_t Deflate::BitReader::readBits(int count) {
    uint32_t result = 0;
    for (int i = 0; i < count; i++) {
        if (byte_pos >= data.size()) {
            throw std::runtime_error("Unexpected end of data");
        }
        result |= ((data[byte_pos] >> bit_pos) & 1) << i;
        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
        }
    }
    return result;
}

uint32_t Deflate::BitReader::readBitsReverse(int count) {
    uint32_t result = 0;
    for (int i = count - 1; i >= 0; i--) {
        if (byte_pos >= data.size()) {
            throw std::runtime_error("Unexpected end of data");
        }
        result |= ((data[byte_pos] >> bit_pos) & 1) << i;
        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
        }
    }
    return result;
}

void Deflate::BitReader::alignToByte() {
    if (bit_pos != 0) {
        bit_pos = 0;
        byte_pos++;
    }
}

bool Deflate::BitReader::hasData() const {
    return byte_pos < data.size();
}

void Deflate::HuffmanTree::build(const std::vector<int>& codeLengths) {
    maxBits = 0;
    for (int len : codeLengths) {
        if (len > maxBits) maxBits = len;
    }
    
    if (maxBits == 0) return;
    
    std::vector<int> blCount(maxBits + 1, 0);
    for (int len : codeLengths) {
        if (len > 0) blCount[len]++;
    }
    
    std::vector<int> nextCode(maxBits + 1, 0);
    int code = 0;
    for (int bits = 1; bits <= maxBits; bits++) {
        code = (code + blCount[bits - 1]) << 1;
        nextCode[bits] = code;
    }
    
    codes.resize(codeLengths.size(), -1);
    lengths = codeLengths;
    
    for (size_t n = 0; n < codeLengths.size(); n++) {
        int len = codeLengths[n];
        if (len != 0) {
            codes[n] = nextCode[len]++;
        }
    }
}

int Deflate::HuffmanTree::decode(BitReader& reader) const {
    int code = 0;
    int first = 0;
    int index = 0;
    
    for (int len = 1; len <= maxBits; len++) {
        code |= reader.readBits(1);
        int count = 0;
        for (size_t i = 0; i < lengths.size(); i++) {
            if (lengths[i] == len) count++;
        }
        if (code - first < count) {
            int j = 0;
            for (size_t i = 0; i < lengths.size(); i++) {
                if (lengths[i] == len) {
                    if (j == code - first) return static_cast<int>(i);
                    j++;
                }
            }
        }
        first = (first + count) << 1;
        code <<= 1;
    }
    throw std::runtime_error("Invalid Huffman code");
}

void Deflate::buildFixedTrees(HuffmanTree& litLen, HuffmanTree& dist) {
    std::vector<int> litLenLengths(288);
    for (int i = 0; i <= 143; i++) litLenLengths[i] = 8;
    for (int i = 144; i <= 255; i++) litLenLengths[i] = 9;
    for (int i = 256; i <= 279; i++) litLenLengths[i] = 7;
    for (int i = 280; i <= 287; i++) litLenLengths[i] = 8;
    litLen.build(litLenLengths);
    
    std::vector<int> distLengths(32, 5);
    dist.build(distLengths);
}

void Deflate::decodeBlock(BitReader& reader, HuffmanTree& litLen, HuffmanTree& dist,
                          std::vector<uint8_t>& output) {
    static const int lengthBase[] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };
    static const int lengthExtra[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };
    static const int distBase[] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577
    };
    static const int distExtra[] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };
    
    while (true) {
        int symbol = litLen.decode(reader);
        
        if (symbol < 256) {
            output.push_back(static_cast<uint8_t>(symbol));
        } else if (symbol == 256) {
            break;
        } else {
            symbol -= 257;
            int length = lengthBase[symbol] + reader.readBits(lengthExtra[symbol]);
            
            int distSymbol = dist.decode(reader);
            int distance = distBase[distSymbol] + reader.readBits(distExtra[distSymbol]);
            
            size_t start = output.size() - distance;
            for (int i = 0; i < length; i++) {
                output.push_back(output[start + i]);
            }
        }
    }
}

std::vector<uint8_t> Deflate::decompress(const std::vector<uint8_t>& data) {
    if (data.size() < 2) {
        throw std::runtime_error("Data too short for zlib header");
    }
    
    // Skip zlib header (2 bytes)
    std::vector<uint8_t> deflateData(data.begin() + 2, data.end() - 4); // Also skip adler32
    
    BitReader reader(deflateData);
    std::vector<uint8_t> output;
    
    bool finalBlock = false;
    while (!finalBlock) {
        finalBlock = reader.readBits(1) != 0;
        int blockType = reader.readBits(2);
        
        if (blockType == 0) {
            // Stored block
            reader.alignToByte();
            if (reader.byte_pos + 4 > deflateData.size()) {
                throw std::runtime_error("Invalid stored block");
            }
            uint16_t len = deflateData[reader.byte_pos] | (deflateData[reader.byte_pos + 1] << 8);
            reader.byte_pos += 4; // Skip len and nlen
            
            for (int i = 0; i < len; i++) {
                output.push_back(deflateData[reader.byte_pos++]);
            }
        } else if (blockType == 1) {
            // Fixed Huffman
            HuffmanTree litLen, dist;
            buildFixedTrees(litLen, dist);
            decodeBlock(reader, litLen, dist, output);
        } else if (blockType == 2) {
            // Dynamic Huffman
            int hlit = reader.readBits(5) + 257;
            int hdist = reader.readBits(5) + 1;
            int hclen = reader.readBits(4) + 4;
            
            static const int codeLengthOrder[] = {
                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
            };
            
            std::vector<int> codeLengthLengths(19, 0);
            for (int i = 0; i < hclen; i++) {
                codeLengthLengths[codeLengthOrder[i]] = reader.readBits(3);
            }
            
            HuffmanTree codeLengthTree;
            codeLengthTree.build(codeLengthLengths);
            
            std::vector<int> allLengths;
            while (allLengths.size() < static_cast<size_t>(hlit + hdist)) {
                int symbol = codeLengthTree.decode(reader);
                
                if (symbol < 16) {
                    allLengths.push_back(symbol);
                } else if (symbol == 16) {
                    int repeat = reader.readBits(2) + 3;
                    int value = allLengths.back();
                    for (int i = 0; i < repeat; i++) {
                        allLengths.push_back(value);
                    }
                } else if (symbol == 17) {
                    int repeat = reader.readBits(3) + 3;
                    for (int i = 0; i < repeat; i++) {
                        allLengths.push_back(0);
                    }
                } else if (symbol == 18) {
                    int repeat = reader.readBits(7) + 11;
                    for (int i = 0; i < repeat; i++) {
                        allLengths.push_back(0);
                    }
                }
            }
            
            std::vector<int> litLenLengths(allLengths.begin(), allLengths.begin() + hlit);
            std::vector<int> distLengths(allLengths.begin() + hlit, allLengths.end());
            
            HuffmanTree litLen, dist;
            litLen.build(litLenLengths);
            dist.build(distLengths);
            
            decodeBlock(reader, litLen, dist, output);
        } else {
            throw std::runtime_error("Invalid block type");
        }
    }
    
    return output;
}