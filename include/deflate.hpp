#ifndef DEFLATE_HPP
#define DEFLATE_HPP

#include <cstdint>
#include <vector>

class Deflate {
public:
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& data);
    
private:
    struct BitReader {
        const std::vector<uint8_t>& data;
        size_t byte_pos;
        int bit_pos;
        
        BitReader(const std::vector<uint8_t>& d) : data(d), byte_pos(0), bit_pos(0) {}
        
        uint32_t readBits(int count);
        uint32_t readBitsReverse(int count);
        void alignToByte();
        bool hasData() const;
    };
    
    struct HuffmanTree {
        std::vector<int> codes;
        std::vector<int> lengths;
        int maxBits;
        
        HuffmanTree() : maxBits(0) {}
        void build(const std::vector<int>& codeLengths);
        int decode(BitReader& reader) const;
    };
    
    static void buildFixedTrees(HuffmanTree& litLen, HuffmanTree& dist);
    static void decodeBlock(BitReader& reader, HuffmanTree& litLen, HuffmanTree& dist, 
                           std::vector<uint8_t>& output);
};

#endif // DEFLATE_HPP