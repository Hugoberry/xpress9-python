#!/usr/bin/env python3
# File Decompression and Validation with Xpress9 - Step by Step

# Step 1: Import the required libraries
import os
import sys
import hashlib
from xpress9 import Xpress9

# Step 2: Define the file paths
# compressed_file = "desired_compression_metadata.sqlitedb"
compressed_file = "out.bin"
original_file = "metadata.sqlitedb"    # The original file for validation
decompressed_file = "metadata_restored.sqlitedb"  # Where to save the decompressed file

# Step 3: Validate that the compressed file exists
if not os.path.exists(compressed_file):
    print(f"Error: Compressed file '{compressed_file}' does not exist.")
    sys.exit(1)

# Step 4: Get the original file size (needed for decompression)
if os.path.exists(original_file):
    original_size = os.path.getsize(original_file)
    print(f"Original file size: {original_size:,} bytes")
else:
    print(f"Warning: Original file '{original_file}' not found for size reference.")
    # If we don't have the original file, we need to know its size from elsewhere
    # For example, this could be stored in a header in the compressed file
    # or passed as an argument to this script
    original_size = int(input("Please enter the original file size in bytes: "))

# Step 5: Read the compressed file
print(f"Reading compressed file: {compressed_file}")
with open(compressed_file, 'rb') as f:
    compressed_data = f.read()

# Step 6: Create a new Xpress9 instance
print("Initializing Xpress9 decompressor")
xpress = Xpress9()

# Step 7: Decompress the data
print(f"Decompressing data (expecting {original_size:,} bytes)...")
decompressed_data = xpress.decompress(compressed_data, original_size)

# Step 8: Verify the size of the decompressed data
decompressed_size = len(decompressed_data)
print(f"Decompressed size: {decompressed_size:,} bytes")

if decompressed_size == original_size:
    print("✓ Size verification passed: Decompressed data has the same size as the original")
else:
    print(f"✗ Size verification failed: Expected {original_size:,} bytes, got {decompressed_size:,} bytes")

# Step 9: Write the decompressed data to the output file
print(f"Writing decompressed data to: {decompressed_file}")
with open(decompressed_file, 'wb') as f:
    f.write(decompressed_data)

# Step 10: Calculate checksums for validation
print("Calculating checksums for validation...")

# First, check if the original file exists for validation
if os.path.exists(original_file):
    # Calculate MD5 hash of the original file
    with open(original_file, 'rb') as f:
        original_hash = hashlib.md5(f.read()).hexdigest()
    print(f"Original file MD5: {original_hash}")
    
    # Calculate MD5 hash of the decompressed file
    decompressed_hash = hashlib.md5(decompressed_data).hexdigest()
    print(f"Decompressed file MD5: {decompressed_hash}")
    
    # Compare the hashes
    if original_hash == decompressed_hash:
        print("✓ Content verification passed: Files are identical")
    else:
        print("✗ Content verification failed: Files are different")
else:
    print(f"Skipping content verification: Original file '{original_file}' not available")

# Step 11: Clean up
print("Cleaning up resources...")
del xpress

# Step 12: Verification of output file
if os.path.exists(decompressed_file):
    output_file_size = os.path.getsize(decompressed_file)
    print(f"Verified output file size: {output_file_size:,} bytes")
    print(f"Decompression complete: '{compressed_file}' → '{decompressed_file}'")
else:
    print("Error: Failed to create decompressed file.")

print("Done!")