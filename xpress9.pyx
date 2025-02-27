# distutils: language = c
# cython: boundscheck=False, wraparound=False, initializedcheck=False

from libc.stdlib cimport malloc, free
from libc.string cimport memcpy
from xpress9 cimport XPRESS9_CONTEXT, Initialize, Terminate, Decompress, Compress, BYTE, UINT, INT

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

    def compress(self, bytes uncompressed_data, int max_compressed_size):
        """
        Compresses the given uncompressed data.
        The max_compressed_size parameter specifies the size of the output buffer.
        Releases the GIL during the compression operation.
        Returns the compressed data as bytes.
        """
        cdef int uncompressed_size = len(uncompressed_data)
        cdef BYTE* uncompressed_ptr = <BYTE*>uncompressed_data
        cdef BYTE* compressed_ptr = <BYTE*>malloc(max_compressed_size)
        if not compressed_ptr:
            raise MemoryError("Failed to allocate output buffer for compression.")

        cdef UINT compressed_size
        with nogil:
            compressed_size = Compress(self.context, uncompressed_ptr, uncompressed_size, compressed_ptr, max_compressed_size)

        # Check for error conditions.
        # Here we consider a return value of 0 or a size that is not smaller than the original as a failure.
        if compressed_size == 0 or compressed_size >= uncompressed_size:
            free(compressed_ptr)
            raise ValueError(f"Compression failed or not effective: uncompressed size {uncompressed_size}, compressed size {compressed_size}")

        compressed_data = bytes(compressed_ptr[:compressed_size])
        free(compressed_ptr)
        return compressed_data
