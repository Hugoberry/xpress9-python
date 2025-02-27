# Xpress9-python

Xpress9-python is a lightweight wrapper for the Xpress9 compression library by Microsoft. This project provides a C extension along with a Cython wrapper, enabling efficient compression and decompression in Python.

## Features

- **Compression:** Compress data using the Xpress9 encoder.
- **Decompression:** Decompress data using the Xpress9 decoder.
- **High Performance:** Optimized with compiler flags and optional OpenMP for multi-threading.
- **Cross-Platform:** Supports Windows, macOS, and Linux.

## Requirements

- Python 3.8 or higher
- Cython (>= 0.29.0)
- setuptools (>= 42)
- wheel
- A C compiler (e.g., gcc, clang, or MSVC)
- OpenMP (optional, for enhanced performance on Linux/Windows)

## Installation

Clone the repository and build the extension:

```bash
git clone https://github.com/Hugoberry/xpress9-python
cd xpress9
python setup.py build_ext --inplace
```

Alternatively, if the package is available on PyPI, install it with pip:

```bash
pip install xpress9
```

## Usage

Below is a simple example demonstrating how to initialize the library, compress data, and then decompress it:

```python
from xpress9 import Xpress9

# Initialize the wrapper
x = Xpress9()
print("Xpress9 initialized successfully!")

# Create some sample, compressible data (e.g., repetitive string)
original_data = b"Hello, world! " * 64
print(f"Original data size: {len(original_data)} bytes")

# Specify the maximum compressed size (adjust as needed)
max_compressed_size = len(original_data)

# Compress the data
compressed_data = x.compress(original_data, max_compressed_size)
print(f"Compressed data size: {len(compressed_data)} bytes")

# Decompress the data back to its original size
decompressed_data = x.decompress(compressed_data, len(original_data))
print(f"Decompressed data size: {len(decompressed_data)} bytes")

# Verify the integrity of the round-trip
if decompressed_data == original_data:
    print("Data round-trip successful!")
else:
    print("Data mismatch!")
```

## License

This project is licensed under the MIT License.

## Acknowledgments

- The underlying Xpress9 compression library is developed by Microsoft and licensed under the MIT License.

