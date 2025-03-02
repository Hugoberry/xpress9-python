#!/usr/bin/env python3
import sys
import time
import random
import string
import argparse
from xpress9 import Xpress9

def generate_test_data(data_type, size):
    """Generate test data of different types."""
    if data_type == "repeated":
        return b"A" * size
    elif data_type == "random":
        return random.randbytes(size)
    elif data_type == "text":
        chars = string.ascii_letters + string.digits + string.punctuation + " \n\t"
        return ''.join(random.choice(chars) for _ in range(size)).encode('utf-8')
    elif data_type == "binary_pattern":
        pattern = bytes([i % 256 for i in range(256)])
        return (pattern * (size // 256 + 1))[:size]
    else:
        raise ValueError(f"Unknown data type: {data_type}")

def run_compression_test(xpress, data, max_compressed_size=None):
    """Run compression and decompression test on the given data."""
    if max_compressed_size is None:
        # Allocate a buffer that's large enough for the worst case
        max_compressed_size = len(data) + 8  # Adding some overhead

    start_time = time.time()
    compressed_data = xpress.compress(data, max_compressed_size)
    compression_time = time.time() - start_time
    
    compression_ratio = len(data) / len(compressed_data) if len(compressed_data) > 0 else 0
    
    start_time = time.time()
    decompressed_data = xpress.decompress(compressed_data, len(data))
    decompression_time = time.time() - start_time
    
    is_valid = decompressed_data == data
    
    return {
        "original_size": len(data),
        "compressed_size": len(compressed_data),
        "compression_ratio": compression_ratio,
        "compression_time": compression_time,
        "decompression_time": decompression_time,
        "is_valid": is_valid
    }

def format_size(size_bytes):
    """Format byte size into human-readable format."""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024 or unit == 'GB':
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024

def run_tests(args):
    """Run all tests with different data types and sizes."""
    xpress = Xpress9()
    passed = 0
    failed = 0
    
    test_configs = [
        {"type": "repeated", "size": 1024, "name": "Small repeated data"},
        {"type": "repeated", "size": 1024 * 1024, "name": "Large repeated data"},
        {"type": "random", "size": 1024, "name": "Small random data"},
        {"type": "random", "size": 1024 * 1024, "name": "Large random data"},
        {"type": "text", "size": 10 * 1024, "name": "Text data"},
        {"type": "binary_pattern", "size": 10 * 1024, "name": "Binary pattern"},
    ]
    
    if args.benchmark:
        # Add larger test cases for benchmarking
        test_configs.extend([
            {"type": "repeated", "size": 10 * 1024 * 1024, "name": "10MB repeated data"},
            {"type": "random", "size": 10 * 1024 * 1024, "name": "10MB random data"},
        ])
    
    print("\n" + "=" * 80)
    print(f"Xpress9 Compression Test Suite")
    print("=" * 80)
    
    for config in test_configs:
        try:
            print(f"\nTest: {config['name']}")
            print("-" * 50)
            
            data = generate_test_data(config["type"], config["size"])
            result = run_compression_test(xpress, data)
            
            print(f"Original size:      {format_size(result['original_size'])}")
            print(f"Compressed size:    {format_size(result['compressed_size'])}")
            print(f"Compression ratio:  {result['compression_ratio']:.2f}x")
            print(f"Compression time:   {result['compression_time'] * 1000:.2f} ms")
            print(f"Decompression time: {result['decompression_time'] * 1000:.2f} ms")
            
            if result["is_valid"]:
                print("Status: PASS - Decompressed data matches original.")
                passed += 1
            else:
                print("Status: FAIL - Decompressed data does not match original!")
                failed += 1
                if not args.continue_on_fail:
                    return False
                
        except Exception as e:
            print(f"Test failed with exception: {e}")
            failed += 1
            if not args.continue_on_fail:
                return False
    
    print("\n" + "=" * 80)
    print(f"Test Summary: {passed} passed, {failed} failed")
    print("=" * 80)
    
    return failed == 0

def edge_cases_test(xpress):
    """Test edge cases like empty data, very small data, etc."""
    print("\nRunning Edge Cases Tests:")
    print("-" * 50)
    
    edge_cases = [
        {"data": b"", "name": "Empty data"},
        {"data": b"A", "name": "Single byte"},
        {"data": b"AB", "name": "Two bytes"},
    ]
    
    for case in edge_cases:
        try:
            print(f"\nEdge Case: {case['name']}")
            data = case["data"]
            max_size = max(len(data) * 2, 8)  # Ensure buffer is big enough
            
            compressed = xpress.compress(data, max_size)
            decompressed = xpress.decompress(compressed, len(data))
            
            if decompressed == data:
                print("Status: PASS")
            else:
                print("Status: FAIL - Data mismatch")
                return False
                
        except Exception as e:
            print(f"Edge case failed with exception: {str(e)}")
            return False
    
    return True

def error_handling_test(xpress):
    """Test error handling of the library."""
    print("\nRunning Error Handling Tests:")
    print("-" * 50)
    
    # Create test data
    test_data = b"A" * 1024
    compressed = xpress.compress(test_data, 1024)
    
    error_cases = [
        {
            "name": "Too small output buffer", 
            "func": lambda: xpress.compress(test_data, 10),
            "expected_error": True
        },
        {
            "name": "Wrong decompression size", 
            "func": lambda: xpress.decompress(compressed, len(test_data) - 100),
            "expected_error": True
        }
    ]
    
    for case in error_cases:
        print(f"\nError Case: {case['name']}")
        try:
            case["func"]()
            if case["expected_error"]:
                print("Status: FAIL - Expected error was not raised")
                return False
            else:
                print("Status: PASS - No error as expected")
        except Exception as e:
            if case["expected_error"]:
                print(f"Status: PASS - Error caught as expected: {str(e)}")
            else:
                print(f"Status: FAIL - Unexpected error: {str(e)}")
                return False
    
    return True

def parse_args():
    parser = argparse.ArgumentParser(description="Xpress9 Compression E2E Test")
    parser.add_argument("--benchmark", action="store_true", help="Run additional benchmarking tests")
    parser.add_argument("--continue-on-fail", action="store_true", help="Continue testing even if a test fails")
    parser.add_argument("--skip-edge-cases", action="store_true", help="Skip edge case tests")
    parser.add_argument("--skip-error-handling", action="store_true", help="Skip error handling tests")
    return parser.parse_args()

def main():
    args = parse_args()
    try:
        xpress = Xpress9()
        
        # Run the main compression tests
        if not run_tests(args):
            print("Main compression tests failed!")
            sys.exit(1)
            
        # Run edge case tests
        if not args.skip_edge_cases:
            if not edge_cases_test(xpress):
                print("Edge case tests failed!")
                sys.exit(1)
                
        # Run error handling tests
        if not args.skip_error_handling:
            if not error_handling_test(xpress):
                print("Error handling tests failed!")
                sys.exit(1)
                
        print("\nAll tests completed successfully!")
        
    except Exception as e:
        print(f"Unexpected error during testing: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()