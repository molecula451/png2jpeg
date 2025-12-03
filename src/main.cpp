#include "png_decoder.hpp"
#include "jpeg_encoder.hpp"
#include <iostream>
#include <string>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "PNG to JPEG Converter (No External Dependencies)\n";
    std::cout << "================================================\n\n";
    std::cout << "Usage: " << programName << " [options] <input. png> [output.jpg]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -q, --quality <1-100>  Set JPEG quality (default: 85)\n";
    std::cout << "  -v, --verbose          Enable verbose output\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "  --version              Show version information\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " image.png\n";
    std::cout << "  " << programName << " image.png output.jpg\n";
    std::cout << "  " << programName << " -q 90 image.png\n";
    std::cout << "  " << programName << " --quality 75 --verbose image.png converted.jpg\n";
}

void printVersion() {
    std::cout << "png2jpg version 1.0.0\n";
    std::cout << "PNG to JPEG converter written in pure C++17\n";
    std::cout << "No external libraries or dependencies\n";
}

std::string getOutputFilename(const std::string& input) {
    size_t dotPos = input.rfind('.');
    if (dotPos != std::string::npos) {
        return input.substr(0, dotPos) + ".jpg";
    }
    return input + ".jpg";
}

int main(int argc, char* argv[]) {
    int quality = 85;
    bool verbose = false;
    std::string inputFile;
    std::string outputFile;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--version") {
            printVersion();
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-q" || arg == "--quality") {
            if (i + 1 < argc) {
                try {
                    quality = std::stoi(argv[++i]);
                    if (quality < 1 || quality > 100) {
                        std::cerr << "Error: Quality must be between 1 and 100\n";
                        return 1;
                    }
                } catch (...) {
                    std::cerr << "Error: Invalid quality value\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: -q/--quality requires a value\n";
                return 1;
            }
        } else if (arg[0] == '-') {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            if (inputFile.empty()) {
                inputFile = arg;
            } else if (outputFile.empty()) {
                outputFile = arg;
            } else {
                std::cerr << "Error: Too many arguments\n";
                printUsage(argv[0]);
                return 1;
            }
        }
    }
    
    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified\n";
        printUsage(argv[0]);
        return 1;
    }
    
    if (outputFile.empty()) {
        outputFile = getOutputFilename(inputFile);
    }
    
    try {
        if (verbose) {
            std::cout << "Input file:  " << inputFile << "\n";
            std::cout << "Output file: " << outputFile << "\n";
            std::cout << "Quality:     " << quality << "\n";
            std::cout << "\nDecoding PNG.. .\n";
        }
        
        Image image = PNGDecoder::decode(inputFile);
        
        if (verbose) {
            std::cout << "Image size:  " << image.width() << "x" << image.height() << "\n";
            std::cout << "Encoding JPEG...\n";
        }
        
        JPEGEncoder::encode(image, outputFile, quality);
        
        if (verbose) {
            std::cout << "Done!\n";
        } else {
            std::cout << "Converted " << inputFile << " -> " << outputFile << "\n";
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
