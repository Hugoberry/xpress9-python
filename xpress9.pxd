# xpress9.pxd

cdef extern from "Xpress9Wrapper.h":
    ctypedef struct XPRESS9_CONTEXT:
        void* decoder
        void* encoder
    ctypedef unsigned int UINT
    ctypedef unsigned char BYTE
    ctypedef int INT

    XPRESS9_CONTEXT* Initialize() nogil
    void Terminate(XPRESS9_CONTEXT* context) nogil
    UINT Decompress(XPRESS9_CONTEXT* context, BYTE* compressed, INT compressedSize, BYTE* original, INT maxOriginalSize) nogil
    UINT Compress(XPRESS9_CONTEXT* context, BYTE* original, INT originalSize, BYTE* compressed, INT maxCompressedSize) nogil
