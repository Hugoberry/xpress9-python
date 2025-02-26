# xpress9.pxd

cdef extern from "Xpress9DLL.h":  # Ensure correct header is included
    ctypedef void* XPRESS9_CONTEXT
    ctypedef unsigned int UINT
    ctypedef unsigned char BYTE
    ctypedef int INT

    XPRESS9_CONTEXT* Initialize() nogil
    void Terminate(XPRESS9_CONTEXT* context) nogil
    UINT Decompress(XPRESS9_CONTEXT* context, BYTE* compressed, INT compressedSize, BYTE* original, INT maxOriginalSize) nogil
