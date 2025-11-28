# distutils: language = c
# cython: boundscheck=False, wraparound=False, initializedcheck=False

from libc.stdlib cimport malloc, free
from libc.string cimport memcpy
from xpress9 cimport XPRESS_CONTEXT, Initialize, Initialize_Default, Terminate, Decompress, Compress, BYTE, UINT, INT, BOOL

cdef class Xpress9:
    """ Python Wrapper for Xpress9 C Library with compression level support """

    cdef XPRESS_CONTEXT* context
    cdef int _compression_level

    def __cinit__(self, int compression_level=9, bint encoder=True, bint decoder=True):
        """ 
        Initialize the library and allocate context.
        
        Args:
            compression_level: Compression level (1-9, default=9)
                               Levels 1-5 use XPress8, levels 6-9 use XPress9
            encoder: Whether to initialize the encoder (True by default)
            decoder: Whether to initialize the decoder (True by default)
        """
        cdef BOOL c_encoder = 1 if encoder else 0
        cdef BOOL c_decoder = 1 if decoder else 0
        
        # Store the compression level
        self._compression_level = compression_level
        
        # Initialize the context with the specified compression level
        self.context = Initialize(c_encoder, c_decoder, compression_level)
        if self.context is NULL:
            raise MemoryError(f"Failed to initialize XPress with compression level {compression_level}.")

    def __dealloc__(self):
        """ Cleanup and free resources. """
        if self.context:
            Terminate(self.context)
            self.context = NULL
    
    @property
    def compression_level(self):
        """ Get the current compression level """
        return self._compression_level

    def decompress(self, bytes compressed_data, int uncompressed_size):
        """ 
        Decompresses the given data. Releases GIL for performance.
        
        Args:
            compressed_data: The compressed data bytes
            uncompressed_size: Expected size of uncompressed data
            
        Returns:
            bytes: Decompressed data
        """
        cdef int compressed_size = len(compressed_data)
        cdef BYTE* compressed_ptr = <BYTE*>compressed_data
        cdef BYTE* decompressed_ptr = <BYTE*>malloc(uncompressed_size)
        if not decompressed_ptr:
            raise MemoryError("Failed to allocate output buffer.")

        cdef UINT decompressed_size
        with nogil:  # Release the GIL to allow parallel execution
            decompressed_size = Decompress(self.context, compressed_ptr, compressed_size, decompressed_ptr, uncompressed_size)

        if decompressed_size == 0:
            free(decompressed_ptr)
            raise ValueError(f"Decompression failed: Got {decompressed_size} bytes")
        
        if decompressed_size != uncompressed_size:
            # This might be a warning rather than an error if your implementation 
            # can handle different sizes
            free(decompressed_ptr)
            raise ValueError(f"Decompression size mismatch: Expected {uncompressed_size}, got {decompressed_size}")

        decompressed_data = bytes(decompressed_ptr[:decompressed_size])
        free(decompressed_ptr)
        return decompressed_data

    def compress(self, bytes uncompressed_data, int max_compressed_size=0):
        """
        Compresses the given uncompressed data.
        The max_compressed_size parameter specifies the size of the output buffer.
        If max_compressed_size is 0, it will be estimated based on input size.
        Releases the GIL during the compression operation.
        
        Args:
            uncompressed_data: The data to compress
            max_compressed_size: Maximum size of compressed output buffer
                                (0 = auto-calculate)
        
        Returns:
            bytes: Compressed data
        """
        cdef int uncompressed_size = len(uncompressed_data)
        
        # Calculate max compressed size if not provided
        if max_compressed_size <= 0:
            # Worst case scenario: compression adds overhead
            max_compressed_size = uncompressed_size + 278
        
        cdef BYTE* uncompressed_ptr = <BYTE*>uncompressed_data
        cdef BYTE* compressed_ptr = <BYTE*>malloc(max_compressed_size)
        if not compressed_ptr:
            raise MemoryError("Failed to allocate output buffer for compression.")

        cdef UINT compressed_size
        with nogil:
            compressed_size = Compress(self.context, uncompressed_ptr, uncompressed_size, compressed_ptr, max_compressed_size)

        # Check for error conditions.
        if compressed_size == 0:
            free(compressed_ptr)
            raise ValueError("Compression failed: buffer may be too small or internal error occurred")

        compressed_data = bytes(compressed_ptr[:compressed_size])
        free(compressed_ptr)
        return compressed_data