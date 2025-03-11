#pragma once
#include <stdlib.h>
#include "xpress.h"
#include "xpress9.h"
#include <stdio.h>

typedef unsigned char BYTE;
typedef int INT;
typedef unsigned int UINT;
typedef void VOID;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define UNREFERENCED_PARAMETER(P) (void)(P)
#define min(a,b) ((a) < (b) ? (a) : (b))

// On Windows, export/import symbols appropriately
#if defined(_MSC_VER)
    #ifdef XPRESS9DLL_EXPORT
        #define XPRESS9DLL_API __declspec(dllexport)
    #elif defined(BUILD_STATIC)  // Check for static builds
        #define XPRESS9DLL_API    
    #else
        #define XPRESS9DLL_API __declspec(dllimport)
    #endif
#else
    #ifdef XPRESS9DLL_EXPORT
        #define XPRESS9DLL_API __attribute__((visibility("default")))
    #elif defined(BUILD_STATIC)
        #define XPRESS9DLL_API
    #else
        #define XPRESS9DLL_API
    #endif
#endif

// Define constants
#define MAX_ORIGINAL_SIZE min(64 * 1024, XPRESS_MAX_BLOCK)
#define XPRESS9_START_COMPRESSION_LEVEL 6

// Forward declaration of memory callbacks
void* XPRESS_CALL XpressAllocMemoryCb(void* Context, int AllocSize);
void XPRESS_CALL XpressFreeMemoryCb(void* Context, void* Address);

typedef struct {
    // Compression level (1-9, default is 9)
    INT compressionLevel;
    
    // XPress8
    XpressEncodeStream encoder8;
    XpressDecodeStream decoder8;
    
    // XPress9
    XPRESS9_ENCODER encoder9;
    XPRESS9_DECODER decoder9;
} XPRESS_CONTEXT;

// Function declarations
XPRESS9DLL_API XPRESS_CONTEXT* Initialize(BOOL encoder, BOOL decoder, INT compressionLevel);
XPRESS9DLL_API VOID Terminate(XPRESS_CONTEXT* context);
XPRESS9DLL_API UINT Decompress(XPRESS_CONTEXT* context, BYTE* compressed, INT compressedSize, BYTE* original, INT maxOriginalSize);
XPRESS9DLL_API UINT Compress(XPRESS_CONTEXT* context, BYTE* original, INT originalSize, BYTE* compressed, INT maxCompressedSize);

// Simplified version that uses default values for backward compatibility
XPRESS9DLL_API XPRESS_CONTEXT* Initialize_Default();