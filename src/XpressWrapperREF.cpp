#include <windows.h>
#include "xpresswrapperref.h"

//
// Initialize the wrapper with the specified compression level.
//
// Arguments
//
//  encoder - Indicates whether the encoder is needed
//
//  decoder - Indicates whether the decoder is needed
//
//
// Return value
//
//  TRUE if initialization succeeded, FALSE if not.
//
BOOL XPressWrapper::Initialize(
    __in BOOL encoder,
    __in BOOL decoder)
{
    BOOL result = FALSE;

    m_compressionLevel = 9;

	// Initialize XPress9
	XPRESS9_STATUS status = {0};

	UINT runtimeFlags = 0;

	if (IsProcessorFeaturePresent (PF_XMMI_INSTRUCTIONS_AVAILABLE) &&
		IsProcessorFeaturePresent (PF_XMMI64_INSTRUCTIONS_AVAILABLE))
	{
		runtimeFlags |= Xpress9Flag_UseSSE2;
	}

	// Initialize XPress9 encoder
	if (encoder)
	{
		m_encoder9 = Xpress9EncoderCreate(
												&status,
												NULL,
												XpressAllocMemoryCb,
												XPRESS9_WINDOW_SIZE_LOG2_MAX,
												runtimeFlags);
		if (m_encoder9 == NULL || status.m_uStatus != Xpress9Status_OK)
		{
			goto exit;
		}

		// Create XPress9 encoding session
		XPRESS9_ENCODER_PARAMS Params = {0};

		Params.m_cbSize = sizeof (Params);
		Params.m_uMaxStreamLength = c_MaxOriginalSize;
		Params.m_uMtfEntryCount = 4;
		Params.m_uLookupDepth = m_compressionLevel;
		Params.m_uOptimizationLevel = 0;
		Params.m_uPtrMinMatchLength = 4;
		Params.m_uMtfMinMatchLength = 2;
		Params.m_uWindowSizeLog2 = min(
								(16 + (m_compressionLevel - c_XPress9StartCompressionLevel) * 2),
								XPRESS9_WINDOW_SIZE_LOG2_MAX);

		Xpress9EncoderStartSession (&status, (XPRESS9_ENCODER) m_encoder9, &Params, TRUE);

		if (status.m_uStatus != Xpress9Status_OK)
		{
			goto exit;
		}
	}

	// Initialize XPress9 decoder
	if (decoder)
	{
		m_decoder9 = Xpress9DecoderCreate(
												&status,
												NULL,
												XpressAllocMemoryCb,
												XPRESS9_WINDOW_SIZE_LOG2_MAX,
												runtimeFlags);

		if (m_decoder9 == NULL || status.m_uStatus != Xpress9Status_OK)
		{
			goto exit;
		}

		Xpress9DecoderStartSession(&status, m_decoder9, TRUE);

		if (status.m_uStatus != Xpress9Status_OK)
		{
			goto exit;
		}
	}

    result = TRUE;

exit:

    return result;
}


//
// Frees the resources associated with the xpress operations.
//
VOID XPressWrapper::Terminate()
{
    XPRESS9_STATUS status = {0};

    if (m_encoder9)
    {
        Xpress9EncoderDestroy(&status, m_encoder9, NULL, XpressFreeMemoryCb);
        m_encoder9 = NULL;
    }

    if (m_decoder9)
    {
        Xpress9DecoderDestroy(&status, m_decoder9, NULL, XpressFreeMemoryCb);
        m_decoder9 = NULL;
    }
}

//
// Compress the data in the specified original buffer.
//
// Arguments
//
//  original - Pointer to the data to compress.
//
//  originalSize - Number of original bytes to compress.
//
//  compressed - Buffer to receive compressed data.
//
//  maxCompressedSize - Size of the compressed buffer.
//
// Return value
//
//  Returns the number of bytes in the compressed buffer. If this is zero or
//  equal to the originalSize, the block cannot be compressed.
//
UINT XPressWrapper::Compress(
    __in_bcount(originalSize) BYTE * original,
    __in INT originalSize,
    __in_bcount(maxCompressedSize) BYTE * compressed,
    __in INT maxCompressedSize)
{
    UINT compressedSize = 0;
    UINT resCompressedSize = 0;
    BOOL detach = FALSE;
    XPRESS9_STATUS status = {0};

	// Compress with XPress9
	Xpress9EncoderAttach(&status, m_encoder9, original, originalSize, TRUE);

	if (status.m_uStatus != Xpress9Status_OK)
	{
		goto exit;
	}

	detach = TRUE;

	for (;;)
	{
		//
		// Repeatedly call compress till there is no more compressed data to return.
		// Note that the original data buffer is supplied only once. The number of repeated calls
		// to compress depends on the size of the intermediate representation and the temporary ouput buffer.
		//
		UINT bytesPromised = Xpress9EncoderCompress(&status, m_encoder9, NULL, NULL);

		if (status.m_uStatus != Xpress9Status_OK)
		{
			goto exit;
		}

		if (bytesPromised == 0)
		{
			// no more compressed data
			break;
		}


		if (bytesPromised > (maxCompressedSize - compressedSize))
		{
			// Compressed data won't fit in the buffer. Return zero and
			// stop the compression.
			goto exit;
		}

		UINT bytesRetrieved = 0;
		UINT isDataAvailable = 0;
		do
		{
			UINT bytesWritten = 0;

			isDataAvailable = Xpress9EncoderFetchCompressedData(
														&status,
														m_encoder9,
														compressed + compressedSize,
														maxCompressedSize - compressedSize,
														&bytesWritten);

			if (status.m_uStatus != Xpress9Status_OK)
			{
				goto exit;
			}

			bytesRetrieved += bytesWritten;
			compressedSize += bytesWritten;
		}
		while (isDataAvailable != 0);

		if (bytesRetrieved != bytesPromised)
		{
			goto exit;
		}
	}

    resCompressedSize = compressedSize;

exit:

    if (detach)
    {
        Xpress9EncoderDetach (&status, m_encoder9, original, originalSize);
    }

    return resCompressedSize;
}


//
// Decompress the data from the specified compress buffer.
//
// Arguments
//
//  compressed - Buffer containing compressed data.
//
//  compressedSize - Number of bytes in the compressed buffer.
//
//  original - Buffer to receive the decompressed data.
//
//  maxOriginalSize - Number of bytes available in the original buffer.
//
// Return value
//
//  Returns the number of bytes in the original buffer. If this is zero the
//  decompress failed.
//
UINT XPressWrapper::Decompress(
    __in_bcount(compressedSize) BYTE * compressed,
    __in INT compressedSize,
    __in_bcount(maxOriginalSize) BYTE * original,
    __in INT maxOriginalSize)
{
    UINT originalSize = 0;
    BOOL detach = FALSE;
    XPRESS9_STATUS status = {0};

    
	// Decompress with XPress9
	Xpress9DecoderAttach(&status, m_decoder9, compressed, compressedSize);

	if (status.m_uStatus != Xpress9Status_OK)
	{
		goto exit;
	}

	detach = TRUE;

	UINT bytesRemaining;

	do
	{
		UINT bytesWritten;
		UINT compressedBytesConsumed;

		bytesRemaining = Xpress9DecoderFetchDecompressedData(
														&status,
														m_decoder9,
														original,
														maxOriginalSize,
														&bytesWritten,
														&compressedBytesConsumed);

		if (status.m_uStatus != Xpress9Status_OK)
		{
			originalSize = 0;
			goto exit;
		}

		if (bytesWritten == 0)
		{
			break;
		}

		originalSize += bytesWritten;
	}
        while (bytesRemaining != 0);

exit:

    if (detach)
    {
        Xpress9DecoderDetach (&status, m_decoder9, compressed, compressedSize);
    }

    return originalSize;

}


