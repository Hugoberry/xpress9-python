#!/usr/bin/env python3
# File Compression with Xpress9 - Step by Step

# Step 1: Import the required libraries
import os
import sys
from xpress9 import Xpress9

# Step 2: Define the input and output file paths
input_file = "metadata.sqlitedb" 
output_file = "out.bin" 

# Step 3: Validate that the input file exists
if not os.path.exists(input_file):
    print(f"Error: Input file '{input_file}' does not exist.")
    sys.exit(1)

# Step 4: Read the input file
print(f"Reading file: {input_file}")
with open(input_file, 'rb') as f:
    input_data = f.read()

# Step 5: Get the size of the original file
original_size = len(input_data)
print(f"Original file size: {original_size:,} bytes")

# Step 6: Create a new Xpress9 instance
print("Initializing Xpress9 compressor")
xpress = Xpress9()

# Step 7: Allocate buffer for the compressed data
# In most cases, compression will reduce size, but we need to handle 
# the worst case where compressed data might be slightly larger
max_compressed_size = original_size + 8  # Adding some overhead for safety
print(f"Allocated buffer size for compression: {max_compressed_size:,} bytes")

# Step 8: Compress the data
print("Compressing data...")
compressed_data = xpress.compress(input_data, max_compressed_size)

# Step 9: Calculate compression statistics
compressed_size = len(compressed_data)
compression_ratio = original_size / compressed_size if compressed_size > 0 else 0
space_saved_percentage = (1 - (compressed_size / original_size)) * 100 if original_size > 0 else 0

print(f"Compressed size: {compressed_size:,} bytes")
print(f"Compression ratio: {compression_ratio:.2f}x")
print(f"Space saved: {space_saved_percentage:.2f}%")

# Step 10: Write the compressed data to the output file
print(f"Writing compressed data to: {output_file}")
with open(output_file, 'wb') as f:
    f.write(compressed_data)

# Step 11: Clean up
print("Cleaning up resources...")
del xpress

# Step 12: Verification
if os.path.exists(output_file):
    output_file_size = os.path.getsize(output_file)
    print(f"Verified output file size: {output_file_size:,} bytes")
    print(f"Compression complete: '{input_file}' → '{output_file}'")
else:
    print("Error: Failed to create output file.")

print("Done!")