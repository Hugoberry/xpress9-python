#include "Xpress9DLL.h"
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

// Initialize creates and returns a decoder instance
XPRESS9DLL_API XPRESS9_CONTEXT* Initialize() {
    XPRESS9_STATUS status = {0};
    UINT runtimeFlags = 0;
    
    // Allocate a context structure
    XPRESS9_CONTEXT* context = (XPRESS9_CONTEXT*)malloc(sizeof(XPRESS9_CONTEXT));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for decoder context\n");
        return NULL;
    }
    
    // Create a decoder and store it in the context
    context->decoder = Xpress9DecoderCreate(&status, NULL, XpressAllocMemoryCb, 
                                           XPRESS9_WINDOW_SIZE_LOG2_MAX, runtimeFlags);
    
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

// Terminate cleans up a specific decoder instance
XPRESS9DLL_API VOID Terminate(XPRESS9_CONTEXT* context) {
    if (context) {
        XPRESS9_STATUS status = {0};
        
        if (context->decoder) {
            Xpress9DecoderDestroy(&status, context->decoder, NULL, XpressFreeMemoryCb);
            context->decoder = NULL;
        }
        
        free(context);
    }
}

// Decompress uses a specific decoder instance
XPRESS9DLL_API UINT Decompress(XPRESS9_CONTEXT* context, BYTE *compressed, INT compressedSize, 
                             BYTE *original, INT maxOriginalSize) {
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
            fprintf(stderr, "Failed during decompression: %s\n", status.m_ErrorDescription);
            return 0;
        }
        
        if (bytesWritten == 0) {
            break;
        }
        
        originalSize += bytesWritten;
    } while (bytesRemaining != 0);
    
    if (detach) {
        Xpress9DecoderDetach(&status, context->decoder, compressed, compressedSize);
    }
    
    return originalSize;
}