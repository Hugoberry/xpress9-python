# distutils: language = c
# cython: boundscheck=False, wraparound=False, initializedcheck=False

from libc.stdlib cimport malloc, free
from libc.string cimport memcpy
from xpress9 cimport XPRESS9_CONTEXT, Initialize, Terminate, Decompress, BYTE, UINT, INT

cdef class Xpress9:
    """ Python Wrapper for Xpress9 C Library """

    cdef XPRESS9_CONTEXT* context

    def __cinit__(self):
        """ Initialize the library and allocate context. """
        self.context = Initialize()
        if self.context is NULL:
            raise MemoryError("Failed to initialize Xpress9.")

    def __dealloc__(self):
        """ Cleanup and free resources. """
        if self.context:
            Terminate(self.context)
            self.context = NULL

    def decompress(self, bytes compressed_data, int uncompressed_size):
        """ Decompresses the given data. Releases GIL for performance. """
        cdef int compressed_size = len(compressed_data)
        cdef BYTE* compressed_ptr = <BYTE*>compressed_data
        cdef BYTE* decompressed_ptr = <BYTE*>malloc(uncompressed_size)

        if not decompressed_ptr:
            raise MemoryError("Failed to allocate output buffer.")

        cdef UINT decompressed_size

        with nogil:  # Release the GIL to allow parallel execution
            decompressed_size = Decompress(self.context, compressed_ptr, compressed_size, decompressed_ptr, uncompressed_size)

        if decompressed_size != uncompressed_size:
            free(decompressed_ptr)
            raise ValueError(f"Decompression failed: Expected {uncompressed_size}, got {decompressed_size}")

        decompressed_data = bytes(decompressed_ptr[:uncompressed_size])
        free(decompressed_ptr)
        return decompressed_data
