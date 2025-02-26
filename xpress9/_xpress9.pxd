# cython: language_level=3
from libc.stdint cimport uint8_t, uint32_t

cdef extern from "xpress9.h":
    # Status handling
    ctypedef struct XPRESS9_STATUS:
        unsigned m_uStatus
        unsigned m_uLineNumber
        const char* m_pFilename
        const char* m_pFunction
        char m_ErrorDescription[1024]
    
    # Constants
    unsigned int Xpress9Status_OK
    unsigned int XPRESS9_WINDOW_SIZE_LOG2_MIN
    unsigned int XPRESS9_WINDOW_SIZE_LOG2_MAX
    
    # Types
    ctypedef void* XPRESS9_DECODER
    
    # Memory callbacks
    ctypedef void* (*XpressAllocFn)(void* Context, int AllocSize)
    ctypedef void (*XpressFreeFn)(void* Context, void* Address)
    
    # Core functions
    XPRESS9_DECODER Xpress9DecoderCreate(
        XPRESS9_STATUS* pStatus,
        void* pAllocContext,
        XpressAllocFn* pAllocFn,
        unsigned uMaxWindowSizeLog2,
        unsigned uFlags
    )
    
    void Xpress9DecoderStartSession(
        XPRESS9_STATUS* pStatus,
        XPRESS9_DECODER pDecoder,
        unsigned fForceReset
    )
    
    void Xpress9DecoderAttach(
        XPRESS9_STATUS* pStatus,
        XPRESS9_DECODER pDecoder,
        const void* pCompData,
        unsigned uCompDataSize
    )
    
    unsigned Xpress9DecoderFetchDecompressedData(
        XPRESS9_STATUS* pStatus,
        XPRESS9_DECODER pDecoder,
        void* pOrigData,
        unsigned uOrigDataSize,
        unsigned* puOrigDataWritten,
        unsigned* puCompDataNeeded
    )
    
    void Xpress9DecoderDetach(
        XPRESS9_STATUS* pStatus,
        XPRESS9_DECODER pDecoder,
        const void* pCompData,
        unsigned uCompDataSize
    )
    
    void Xpress9DecoderDestroy(
        XPRESS9_STATUS* pStatus,
        XPRESS9_DECODER pDecoder,
        void* pFreeContext,
        XpressFreeFn* pFreeFn
    )

cdef class Xpress9:
    cdef XPRESS9_DECODER decoder
    cdef bint initialized
    cdef void* _allocate(self, int size) nogil
    cdef void _free(self, void* ptr) nogil