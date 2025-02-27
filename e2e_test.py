#!/usr/bin/env python3
import sys
from xpress9 import Xpress9

def main():
    # Create some compressible data: e.g., 1024 bytes of the character 'A'
    original_data = b"A" * 1024
    print(f"Original data size: {len(original_data)} bytes")

    # Create an instance of the wrapper
    xpress = Xpress9()

    # Set an output buffer size. This value should be large enough to hold the compressed data.
    # For compressible data, the compressed size is expected to be smaller than the original.
    max_compressed_size = 1024  # adjust if necessary

    try:
        # Compress the data
        compressed_data = xpress.compress(original_data, max_compressed_size)
        print(f"Compressed data size: {len(compressed_data)} bytes")

        # Decompress the data back, specifying the original uncompressed size
        decompressed_data = xpress.decompress(compressed_data, len(original_data))
        print(f"Decompressed data size: {len(decompressed_data)} bytes")

        # Verify that the decompressed data matches the original data
        if decompressed_data == original_data:
            print("Test successful: Decompressed data matches original.")
        else:
            print("Test failed: Decompressed data does not match original.")
            sys.exit(1)
    except Exception as e:
        print("An error occurred during testing:", e)
        sys.exit(1)

if __name__ == "__main__":
    main()
