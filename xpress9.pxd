# xpress9.pxd

cdef extern from "Xpress9Wrapper.h":
    ctypedef int BOOL
    ctypedef unsigned int UINT
    ctypedef unsigned char BYTE
    ctypedef int INT
    
    # Updated context structure
    ctypedef struct XPRESS_CONTEXT:
        INT compressionLevel
        void* encoder8
        void* decoder8
        void* encoder9
        void* decoder9
    
    # Original function for backward compatibility
    XPRESS_CONTEXT* Initialize_Default() nogil
    
    # Functions with compression level support
    XPRESS_CONTEXT* Initialize(BOOL encoder, BOOL decoder, INT compressionLevel) nogil
    void Terminate(XPRESS_CONTEXT* context) nogil
    UINT Decompress(XPRESS_CONTEXT* context, BYTE* compressed, INT compressedSize, BYTE* original, INT maxOriginalSize) nogil
    UINT Compress(XPRESS_CONTEXT* context, BYTE* original, INT originalSize, BYTE* compressed, INT maxCompressedSize) nogil