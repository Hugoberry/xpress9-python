#pragma once 
#include <stdlib.h>
#include "xpress.h"
#include "xpress9.h"

//
// Lightweight wrapper on top of XPress library. Supports both XPRESS8 and
// XPRESS9.
// Compression levels 1 to 5 invoke XPRESS8.
// Compression levels 6 to 9 invoke XPRESS9.
//
class XPressWrapper
{
public:

    static const INT c_MaxOriginalSize = min(64 * 1024, XPRESS_MAX_BLOCK);
    static const INT c_XPress9StartCompressionLevel = 6;

public:

    XPressWrapper()
    {
        m_compressionLevel = 0;
        m_encoder8 = NULL;
        m_decoder8 = NULL;
        m_encoder9 = NULL;
        m_decoder9 = NULL;
    }

    ~XPressWrapper()
    {
        Terminate();
    }

public:

    BOOL Initialize(
        __in BOOL encoder,
        __in BOOL decoder,
        __in INT compressionLevel);

    VOID Terminate();

    UINT Compress(
        __in_bcount(originalSize) BYTE * original,
        __in INT originalSize,
        __in_bcount(maxCompressedSize) BYTE * compressed,
        __in INT maxCompressedSize);

    UINT Decompress(
        __in_bcount(compressedSize) BYTE * compressed,
        __in INT compressedSize,
        __in_bcount(maxOriginalSize) BYTE * original,
        __in INT maxOriginalSize);

private:

    // Compression level to apply. Also used to select the compression level.
    INT m_compressionLevel;

    // XPress8
    XpressEncodeStream m_encoder8;
    XpressDecodeStream m_decoder8;

    // XPress9
    XPRESS9_ENCODER m_encoder9;
    XPRESS9_DECODER m_decoder9;

private:

    static void * XPRESS_CALL XpressAllocMemoryCb(void *Context, int AllocSize)
    {
        UNREFERENCED_PARAMETER(Context);
        return malloc(AllocSize);
    }

    static void XPRESS_CALL XpressFreeMemoryCb(void *Context, void *Address)
    {
        UNREFERENCED_PARAMETER(Context);
        free(Address);
    }
};
