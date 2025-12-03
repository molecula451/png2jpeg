# PNG to JPEG Converter


<img width="1103" height="631" alt="png2jpeg" src="https://github.com/user-attachments/assets/2cbe157c-871f-478d-ac13-aac9243a1c52" />

A fast, modern C++17 PNG to JPEG converter with **zero external dependencies**.

## Features

- Pure C++17 implementation
- No external libraries required (no libpng, libjpeg, etc.)
- Supports 8-bit PNG images (grayscale, RGB, RGBA)
- Adjustable JPEG quality (1-100)
- Cross-platform (Linux, macOS, Windows)

## Building

### Requirements

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

### Compile

```bash
# Clone the repository
git clone https://github.com/molecula451/png2jpeg.git
cd png2jpeg

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# (Optional) Install
sudo make install
```

### Windows (Visual Studio)

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Usage

```bash
# Basic usage (output will be input.jpg)
./png2jpg input.png

# Specify output filename
./png2jpg input.png output.jpg

# Set JPEG quality (1-100, default: 85)
./png2jpg -q 90 input.png

# Verbose output
./png2jpg -v input.png output.jpg

# Show help
./png2jpg --help

# Show version
./png2jpg --version
```

### Options

| Option | Description |
|--------|-------------|
| `-q, --quality <1-100>` | Set JPEG quality (default: 85) |
| `-v, --verbose` | Enable verbose output |
| `-h, --help` | Show help message |
| `--version` | Show version information |

### Examples

```bash
# Convert with default quality (85)
./png2jpg photo.png

# High quality conversion
./png2jpg -q 95 photo.png photo_hq.jpg

# Lower quality for smaller file size
./png2jpg --quality 60 --verbose screenshot.png compressed.jpg
```

## Limitations

- Only supports 8-bit depth PNG images
- Interlaced PNGs are not supported
- Alpha channel is ignored during conversion

## License

See [LICENSE](LICENSE) for details.
