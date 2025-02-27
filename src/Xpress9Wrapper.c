#include "Xpress9Wrapper.h"
#include <stdio.h>

// Memory management callbacks
void* XPRESS_CALL XpressAllocMemoryCb(void *Context, int AllocSize)
{
    UNREFERENCED_PARAMETER(Context);
    return malloc(AllocSize);
}

void XPRESS_CALL XpressFreeMemoryCb(void *Context, void *Address)
{
    UNREFERENCED_PARAMETER(Context);
    free(Address);
}

// Initialize creates and returns a context instance with a decoder.
// The encoder is not created until a compress call is made.
XPRESS9DLL_API XPRESS9_CONTEXT* Initialize() {
    XPRESS9_STATUS status = {0};

    // Allocate a context structure
    XPRESS9_CONTEXT* context = (XPRESS9_CONTEXT*)malloc(sizeof(XPRESS9_CONTEXT));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for context\n");
        return NULL;
    }
    context->decoder = NULL;
    context->encoder = NULL;

    // Create a decoder and store it in the context
    context->decoder = Xpress9DecoderCreate(&status, NULL, XpressAllocMemoryCb, 
                                             XPRESS9_WINDOW_SIZE_LOG2_MAX, 0);
    if (context->decoder == NULL || status.m_uStatus != Xpress9Status_OK) {
        fprintf(stderr, "Failed to initialize XPress9 decoder: %s\n", status.m_ErrorDescription);
        free(context);
        return NULL;
    }

    // Start a session for this decoder
    Xpress9DecoderStartSession(&status, context->decoder, 1);
    if (status.m_uStatus != Xpress9Status_OK) {
        fprintf(stderr, "Failed to start XPress9 decoder session: %s\n", status.m_ErrorDescription);
        Xpress9DecoderDestroy(&status, context->decoder, NULL, XpressFreeMemoryCb);
        free(context);
        return NULL;
    }
    
    return context;
}

// Terminate cleans up the decoder and encoder (if any) in the given context.
XPRESS9DLL_API VOID Terminate(XPRESS9_CONTEXT* context) {
    if (context) {
        XPRESS9_STATUS status = {0};

        if (context->decoder) {
            Xpress9DecoderDestroy(&status, context->decoder, NULL, XpressFreeMemoryCb);
            context->decoder = NULL;
        }
        if (context->encoder) {
            Xpress9EncoderDestroy(&status, context->encoder, NULL, XpressFreeMemoryCb);
            context->encoder = NULL;
        }
        free(context);
    }
}

// Decompress uses a specific decoder instance.
XPRESS9DLL_API UINT Decompress(XPRESS9_CONTEXT* context, BYTE *compressed, INT compressedSize, BYTE *original, INT maxOriginalSize) {
    if (!context || !context->decoder) {
        fprintf(stderr, "Invalid decoder context\n");
        return 0;
    }
    
    UINT originalSize = 0;
    int detach = 0;
    XPRESS9_STATUS status = {0};
    
    Xpress9DecoderAttach(&status, context->decoder, compressed, compressedSize);
    if (status.m_uStatus != Xpress9Status_OK) {
        fprintf(stderr, "Failed to attach XPress9 decoder: %s\n", status.m_ErrorDescription);
        return 0;
    }
    
    detach = 1;
    UINT bytesRemaining;
    
    do {
        UINT bytesWritten;
        UINT compressedBytesConsumed;
        
        bytesRemaining = Xpress9DecoderFetchDecompressedData(&status, context->decoder, 
                                                               original, maxOriginalSize, 
                                                               &bytesWritten, &compressedBytesConsumed);
        
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
        Xpress9DecoderDetach(&status, context->decoder, compressed, compressedSize);
    }
    
    return originalSize;
}

// Compress the data in the original buffer and write the compressed data to the compressed buffer.
// Returns the number of bytes of compressed data.
// If zero is returned or if the output buffer is too small, the compression has failed.
XPRESS9DLL_API UINT Compress(XPRESS9_CONTEXT* context, BYTE *original, INT originalSize, BYTE *compressed, INT maxCompressedSize) {
    if (!context || !original || !compressed) {
        fprintf(stderr, "Invalid parameters to Compress\n");
        return 0;
    }

    XPRESS9_STATUS status = {0};

    // Create an encoder if not already created.
    if (context->encoder == NULL) {
        UINT runtimeFlags = 0;
        context->encoder = Xpress9EncoderCreate(&status, NULL, XpressAllocMemoryCb, XPRESS9_WINDOW_SIZE_LOG2_MAX, runtimeFlags);
        if (context->encoder == NULL || status.m_uStatus != Xpress9Status_OK) {
            fprintf(stderr, "Failed to create XPress9 encoder: %s\n", status.m_ErrorDescription);
            return 0;
        }
        // Set up encoder parameters.
        XPRESS9_ENCODER_PARAMS params = {0};
        params.m_cbSize = sizeof(params);
        // Here we use MAX_ORIGINAL_SIZE (or you could use originalSize) as the maximum stream length.
        params.m_uMaxStreamLength = MAX_ORIGINAL_SIZE;
        // Use a compression level of 9 (highest quality). For XPRESS9, levels 6–9 use this encoder.
        int compressionLevel = 9;
        params.m_uMtfEntryCount = 4;  // Use Move-To-Front with 4 entries
        params.m_uLookupDepth = compressionLevel;
        params.m_uOptimizationLevel = 0;
        params.m_uPtrMinMatchLength = 4;
        params.m_uMtfMinMatchLength = 2;
        // Calculate window size: for example, min(16 + (compressionLevel-6)*2, XPRESS9_WINDOW_SIZE_LOG2_MAX)
        int calculatedWindowSize = 16 + (compressionLevel - 6) * 2;
        if (calculatedWindowSize > XPRESS9_WINDOW_SIZE_LOG2_MAX)
            calculatedWindowSize = XPRESS9_WINDOW_SIZE_LOG2_MAX;
        params.m_uWindowSizeLog2 = calculatedWindowSize;

        Xpress9EncoderStartSession(&status, context->encoder, &params, 1);
        if (status.m_uStatus != Xpress9Status_OK) {
            fprintf(stderr, "Failed to start XPress9 encoder session: %s\n", status.m_ErrorDescription);
            return 0;
        }
    }

    // Attach the original (uncompressed) data to the encoder.
    Xpress9EncoderAttach(&status, context->encoder, original, originalSize, 1);
    if (status.m_uStatus != Xpress9Status_OK) {
        fprintf(stderr, "Failed to attach original data to encoder: %s\n", status.m_ErrorDescription);
        return 0;
    }

    UINT totalCompressedSize = 0;
    // Loop: call the compress routine until no more compressed data is available.
    while (1) {
        UINT bytesPromised = Xpress9EncoderCompress(&status, context->encoder, NULL, NULL);
        if (status.m_uStatus != Xpress9Status_OK) {
            fprintf(stderr, "Error during compression: %s\n", status.m_ErrorDescription);
            totalCompressedSize = 0;
            goto detach;
        }
        if (bytesPromised == 0) {
            // No more data to compress.
            break;
        }
        if (totalCompressedSize + bytesPromised > (UINT)maxCompressedSize) {
            fprintf(stderr, "Compressed data exceeds maximum allowed size.\n");
            totalCompressedSize = 0;
            goto detach;
        }
        UINT bytesWritten = 0;
        UINT isDataAvailable = 0;
        do {
            UINT fetched = 0;
            isDataAvailable = Xpress9EncoderFetchCompressedData(&status, context->encoder,
                                                                compressed + totalCompressedSize,
                                                                maxCompressedSize - totalCompressedSize,
                                                                &fetched);
            if (status.m_uStatus != Xpress9Status_OK) {
                fprintf(stderr, "Error fetching compressed data: %s\n", status.m_ErrorDescription);
                totalCompressedSize = 0;
                goto detach;
            }
            bytesWritten += fetched;
            totalCompressedSize += fetched;
        } while (isDataAvailable != 0);

        if (bytesWritten != bytesPromised) {
            fprintf(stderr, "Mismatch in compressed data size.\n");
            totalCompressedSize = 0;
            goto detach;
        }
    }

detach:
    // Detach the original data.
    Xpress9EncoderDetach(&status, context->encoder, original, originalSize);
    return totalCompressedSize;
}
