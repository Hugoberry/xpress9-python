#include "Xpress9Wrapper.h"

// Memory management callbacks
void* XPRESS_CALL XpressAllocMemoryCb(void* Context, int AllocSize)
{
    UNREFERENCED_PARAMETER(Context);
    return malloc(AllocSize);
}

void XPRESS_CALL XpressFreeMemoryCb(void* Context, void* Address)
{
    UNREFERENCED_PARAMETER(Context);
    free(Address);
}

// Initialize with default values for backward compatibility
XPRESS9DLL_API XPRESS_CONTEXT* Initialize_Default() {
    // Default: decoder enabled, encoder lazy-initialized, level 9
    return Initialize(TRUE, TRUE, 9);
}

// Initialize creates and returns a context instance
XPRESS9DLL_API XPRESS_CONTEXT* Initialize(BOOL encoder, BOOL decoder, INT compressionLevel) {
    // Allocate a context structure
    XPRESS_CONTEXT* context = (XPRESS_CONTEXT*)malloc(sizeof(XPRESS_CONTEXT));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for context\n");
        return NULL;
    }
    
    // Initialize all pointers to NULL
    context->encoder8 = NULL;
    context->decoder8 = NULL;
    context->encoder9 = NULL;
    context->decoder9 = NULL;
    
    // Set compression level (default to 9 if invalid)
    if (compressionLevel < 1 || compressionLevel > 9) {
        context->compressionLevel = 9;
    } else {
        context->compressionLevel = compressionLevel;
    }
    
    // Determine whether to use XPress8 or XPress9 based on compression level
    BOOL useXpress8 = (context->compressionLevel < XPRESS9_START_COMPRESSION_LEVEL);
    
    if (useXpress8) {
        // Initialize XPress8
        if (decoder) {
            context->decoder8 = XpressDecodeCreate(NULL, XpressAllocMemoryCb);
            if (context->decoder8 == NULL) {
                Terminate(context);
                return NULL;
            }
        }
        
        if (encoder) {
            context->encoder8 = XpressEncodeCreate(
                MAX_ORIGINAL_SIZE,
                NULL,
                XpressAllocMemoryCb,
                context->compressionLevel);
            
            if (context->encoder8 == NULL) {
                Terminate(context);
                return NULL;
            }
        }
    } else {
        // Initialize XPress9
        XPRESS9_STATUS status = {0};
        
        if (decoder) {
            // Create a decoder and store it in the context
            context->decoder9 = Xpress9DecoderCreate(
                &status,
                NULL,
                XpressAllocMemoryCb,
                XPRESS9_WINDOW_SIZE_LOG2_MAX,
                0);
                
            if (context->decoder9 == NULL || status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to initialize XPress9 decoder: %s\n", status.m_ErrorDescription);
                Terminate(context);
                return NULL;
            }
            
            // Start a session for this decoder
            Xpress9DecoderStartSession(&status, context->decoder9, TRUE);
            if (status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to start XPress9 decoder session: %s\n", status.m_ErrorDescription);
                Terminate(context);
                return NULL;
            }
        }
        
        if (encoder) {
            // Create an encoder and store it in the context
            context->encoder9 = Xpress9EncoderCreate(
                &status,
                NULL,
                XpressAllocMemoryCb,
                XPRESS9_WINDOW_SIZE_LOG2_MAX,
                0);
                
            if (context->encoder9 == NULL || status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to initialize XPress9 encoder: %s\n", status.m_ErrorDescription);
                Terminate(context);
                return NULL;
            }
            
            // Create XPress9 encoding session with appropriate parameters
            XPRESS9_ENCODER_PARAMS params = {0};
            params.m_cbSize = sizeof(params);
            params.m_uMaxStreamLength = MAX_ORIGINAL_SIZE;
            params.m_uMtfEntryCount = 4;  // Use Move-To-Front with 4 entries
            params.m_uLookupDepth = context->compressionLevel;
            params.m_uOptimizationLevel = 0;
            params.m_uPtrMinMatchLength = 4;
            params.m_uMtfMinMatchLength = 2;
            
            // Calculate window size based on compression level
            int windowSizeLog2 = min(
                (16 + (context->compressionLevel - XPRESS9_START_COMPRESSION_LEVEL) * 2),
                XPRESS9_WINDOW_SIZE_LOG2_MAX);
            params.m_uWindowSizeLog2 = windowSizeLog2;
            
            Xpress9EncoderStartSession(&status, context->encoder9, &params, TRUE);
            if (status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to start XPress9 encoder session: %s\n", status.m_ErrorDescription);
                Terminate(context);
                return NULL;
            }
        }
    }
    
    return context;
}

// Terminate cleans up the decoder and encoder (if any) in the given context.
XPRESS9DLL_API VOID Terminate(XPRESS_CONTEXT* context) {
    if (context) {
        XPRESS9_STATUS status = {0};
        
        // Clean up XPress8 components
        if (context->encoder8) {
            XpressEncodeClose(context->encoder8, NULL, XpressFreeMemoryCb);
            context->encoder8 = NULL;
        }
        
        if (context->decoder8) {
            XpressDecodeClose(context->decoder8, NULL, XpressFreeMemoryCb);
            context->decoder8 = NULL;
        }
        
        // Clean up XPress9 components
        if (context->encoder9) {
            Xpress9EncoderDestroy(&status, context->encoder9, NULL, XpressFreeMemoryCb);
            context->encoder9 = NULL;
        }
        
        if (context->decoder9) {
            Xpress9DecoderDestroy(&status, context->decoder9, NULL, XpressFreeMemoryCb);
            context->decoder9 = NULL;
        }
        
        free(context);
    }
}

// Compress the data using either XPress8 or XPress9 based on the configuration
XPRESS9DLL_API UINT Compress(XPRESS_CONTEXT* context, BYTE* original, INT originalSize, BYTE* compressed, INT maxCompressedSize) {
    if (!context || !original || !compressed) {
        fprintf(stderr, "Invalid parameters to Compress\n");
        return 0;
    }

    UINT compressedSize = 0;
    
    // If we're using XPress8 (compression levels 1-5)
    if (context->compressionLevel < XPRESS9_START_COMPRESSION_LEVEL) {
        if (context->encoder8 == NULL) {
            // Lazy initialization if needed
            XPRESS9_STATUS status = {0}; // Unused but kept for consistency
            context->encoder8 = XpressEncodeCreate(
                MAX_ORIGINAL_SIZE,
                NULL,
                XpressAllocMemoryCb,
                context->compressionLevel);
                
            if (context->encoder8 == NULL) {
                fprintf(stderr, "Failed to initialize XPress8 encoder\n");
                return 0;
            }
        }
        
        // Compress with XPress8
        compressedSize = XpressEncode(
            context->encoder8,
            compressed,
            maxCompressedSize,
            original,
            originalSize,
            NULL,
            0,
            0);
    } 
    // Otherwise, use XPress9 (compression levels 6-9)
    else {
        XPRESS9_STATUS status = {0};
        BOOL detach = FALSE;
        
        // Create an encoder if not already created
        if (context->encoder9 == NULL) {
            context->encoder9 = Xpress9EncoderCreate(
                &status,
                NULL,
                XpressAllocMemoryCb,
                XPRESS9_WINDOW_SIZE_LOG2_MAX,
                0);
                
            if (context->encoder9 == NULL || status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to create XPress9 encoder: %s\n", status.m_ErrorDescription);
                return 0;
            }
            
            // Set up encoder parameters
            XPRESS9_ENCODER_PARAMS params = {0};
            params.m_cbSize = sizeof(params);
            params.m_uMaxStreamLength = MAX_ORIGINAL_SIZE;
            params.m_uMtfEntryCount = 4;
            params.m_uLookupDepth = context->compressionLevel;
            params.m_uOptimizationLevel = 0;
            params.m_uPtrMinMatchLength = 4;
            params.m_uMtfMinMatchLength = 2;
            
            // Calculate window size based on compression level
            int windowSizeLog2 = min(
                (16 + (context->compressionLevel - XPRESS9_START_COMPRESSION_LEVEL) * 2),
                XPRESS9_WINDOW_SIZE_LOG2_MAX);
            params.m_uWindowSizeLog2 = windowSizeLog2;
            
            Xpress9EncoderStartSession(&status, context->encoder9, &params, TRUE);
            if (status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to start XPress9 encoder session: %s\n", status.m_ErrorDescription);
                return 0;
            }
        }
        
        // Attach the original data to the encoder
        Xpress9EncoderAttach(&status, context->encoder9, original, originalSize, TRUE);
        if (status.m_uStatus != Xpress9Status_OK) {
            fprintf(stderr, "Failed to attach original data to encoder: %s\n", status.m_ErrorDescription);
            return 0;
        }
        
        detach = TRUE;
        
        // Compress loop - similar to your existing Compress function
        for (;;) {
            UINT bytesPromised = Xpress9EncoderCompress(&status, context->encoder9, NULL, NULL);
            
            if (status.m_uStatus != Xpress9Status_OK) {
                compressedSize = 0;
                goto exit;
            }
            
            if (bytesPromised == 0) {
                // No more data to compress
                break;
            }
            
            if (bytesPromised > (maxCompressedSize - compressedSize)) {
                // Compressed data won't fit in the buffer
                compressedSize = 0;
                goto exit;
            }
            
            UINT bytesRetrieved = 0;
            UINT isDataAvailable = 0;
            
            do {
                UINT bytesWritten = 0;
                
                isDataAvailable = Xpress9EncoderFetchCompressedData(
                    &status,
                    context->encoder9,
                    compressed + compressedSize,
                    maxCompressedSize - compressedSize,
                    &bytesWritten);
                    
                if (status.m_uStatus != Xpress9Status_OK) {
                    compressedSize = 0;
                    goto exit;
                }
                
                bytesRetrieved += bytesWritten;
                compressedSize += bytesWritten;
            } while (isDataAvailable != 0);
            
            if (bytesRetrieved != bytesPromised) {
                compressedSize = 0;
                goto exit;
            }
        }
        
    exit:
        if (detach) {
            Xpress9EncoderDetach(&status, context->encoder9, original, originalSize);
        }
    }
    
    return compressedSize;
}

// Decompress using either XPress8 or XPress9 based on the implementation
XPRESS9DLL_API UINT Decompress(XPRESS_CONTEXT* context, BYTE* compressed, INT compressedSize, BYTE* original, INT maxOriginalSize) {
    if (!context || !compressed || !original) {
        fprintf(stderr, "Invalid parameters to Decompress\n");
        return 0;
    }
    
    UINT originalSize = 0;
    
    // Check compressed data format to determine the algorithm to use
    // In real implementation, you might need to examine the header bytes
    // to determine if it's XPress8 or XPress9 data
    BOOL isXpress9 = FALSE;
    
    // Simple heuristic - check the first few bytes of the header
    // Note: This is a simplification - actual format detection would depend on
    // the specific header format of your compressed data
    if (compressedSize > 0) {
        // XPress9 often has a signature in the first byte(s)
        // Adjust this based on your actual format
        isXpress9 = ((compressed[0] & 0xF0) == 0xF0);
    }
    
    // Use appropriate decompressor based on format detection or context configuration
    if (!isXpress9 && context->decoder8 != NULL) {
        // Decompress with XPress8
        originalSize = XpressDecode(
            context->decoder8,
            original,
            maxOriginalSize,
            maxOriginalSize,
            compressed,
            compressedSize);
    } else {
        // Ensure we have a decoder9
        if (context->decoder9 == NULL) {
            XPRESS9_STATUS status = {0};
            
            // Create decoder on demand
            context->decoder9 = Xpress9DecoderCreate(
                &status,
                NULL,
                XpressAllocMemoryCb,
                XPRESS9_WINDOW_SIZE_LOG2_MAX,
                0);
                
            if (context->decoder9 == NULL || status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to initialize XPress9 decoder: %s\n", status.m_ErrorDescription);
                return 0;
            }
            
            // Start a session
            Xpress9DecoderStartSession(&status, context->decoder9, TRUE);
            if (status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Failed to start XPress9 decoder session: %s\n", status.m_ErrorDescription);
                return 0;
            }
        }
        
        // Decompress with XPress9
        XPRESS9_STATUS status = {0};
        BOOL detach = FALSE;
        
        Xpress9DecoderAttach(&status, context->decoder9, compressed, compressedSize);
        if (status.m_uStatus != Xpress9Status_OK) {
            fprintf(stderr, "Failed to attach XPress9 decoder: %s\n", status.m_ErrorDescription);
            return 0;
        }
        
        detach = TRUE;
        UINT bytesRemaining;
        
        do {
            UINT bytesWritten;
            UINT compressedBytesConsumed;
            
            bytesRemaining = Xpress9DecoderFetchDecompressedData(
                &status,
                context->decoder9,
                original + originalSize,
                maxOriginalSize - originalSize,
                &bytesWritten,
                &compressedBytesConsumed);
                
            if (status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Error during decompression: %s\n", status.m_ErrorDescription);
                originalSize = 0;
                goto exit;
            }
            
            if (bytesWritten == 0) {
                break;
            }
            
            originalSize += bytesWritten;
        } while (bytesRemaining != 0);
        
    exit:
        if (detach) {
            Xpress9DecoderDetach(&status, context->decoder9, compressed, compressedSize);
        }
    }
    
    return originalSize;
}