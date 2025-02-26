# cython: language_level=3

from libc.stdlib cimport malloc, free
from cpython.mem cimport PyMem_Malloc, PyMem_Free
from cpython.bytes cimport PyBytes_FromStringAndSize
from libc.string cimport memcpy

cimport _xpress9

# Memory allocation functions for Xpress9
cdef void* xpress_alloc(void* context, int size) with gil:
    return malloc(size)

cdef void xpress_free(void* context, void* ptr) with gil:
    free(ptr)

cdef class Xpress9:
    """
    A Cython wrapper for the Xpress9 compression library.
    """
    
    def __cinit__(self):
        """Initialize the wrapper attributes."""
        self.decoder = NULL
        self.initialized = False
    
    def __dealloc__(self):
        """Clean up resources when the object is destroyed."""
        self.terminate()
    
    cdef void* _allocate(self, int size) nogil:
        """Allocate memory for internal use."""
        return malloc(size)
    
    cdef void _free(self, void* ptr) nogil:
        """Free memory for internal use."""
        if ptr != NULL:
            free(ptr)
    
    def initialize(self, window_size_log2=None):
        """
        Initialize the Xpress9 library.
        
        Args:
            window_size_log2 (int, optional): Log2 of the window size.
                Should be between XPRESS9_WINDOW_SIZE_LOG2_MIN and 
                XPRESS9_WINDOW_SIZE_LOG2_MAX. Defaults to max.
        
        Raises:
            RuntimeError: If initialization fails.
            ValueError: If window_size_log2 is out of range.
        """
        if self.initialized:
            return
        
        # Use maximum window size by default
        if window_size_log2 is None:
            window_size_log2 = XPRESS9_WINDOW_SIZE_LOG2_MAX
        
        # Validate window size
        if not (XPRESS9_WINDOW_SIZE_LOG2_MIN <= window_size_log2 <= XPRESS9_WINDOW_SIZE_LOG2_MAX):
            raise ValueError(
                f"Window size log2 must be between {XPRESS9_WINDOW_SIZE_LOG2_MIN} "
                f"and {XPRESS9_WINDOW_SIZE_LOG2_MAX}"
            )
        
        cdef:
            XPRESS9_STATUS status
            XpressAllocFn alloc_fn = xpress_alloc
            unsigned int flags = 0  # No flags needed for basic operation
        
        # Initialize status
        status.m_uStatus = 0
        
        # Create decoder
        self.decoder = Xpress9DecoderCreate(
            &status,
            NULL,  # No allocation context needed
            &alloc_fn,
            window_size_log2,
            flags
        )
        
        if self.decoder is NULL or status.m_uStatus != Xpress9Status_OK:
            error_msg = f"Failed to initialize Xpress9: {status.m_ErrorDescription.decode('utf-8')}"
            raise RuntimeError(error_msg)
        
        # Start a session
        Xpress9DecoderStartSession(
            &status,
            self.decoder,
            1  # Force reset
        )
        
        if status.m_uStatus != Xpress9Status_OK:
            error_msg = f"Failed to start Xpress9 session: {status.m_ErrorDescription.decode('utf-8')}"
            self._destroy_decoder()
            raise RuntimeError(error_msg)
        
        self.initialized = True
    
    def _destroy_decoder(self):
        """Internal method to destroy the decoder."""
        if self.decoder is not NULL:
            cdef:
                XPRESS9_STATUS status
                XpressFreeFn free_fn = xpress_free
            
            Xpress9DecoderDestroy(&status, self.decoder, NULL, &free_fn)
            self.decoder = NULL
    
    def terminate(self):
        """
        Terminate the Xpress9 library.
        
        This method is automatically called when the object is destroyed,
        but can be called manually to free resources earlier.
        """
        if self.initialized:
            self._destroy_decoder()
            self.initialized = False
    
    def decompress(self, bytes compressed_data, unsigned int expected_size):
        """
        Decompress data using the Xpress9 library.
        
        Args:
            compressed_data (bytes): The compressed data
            expected_size (int): Expected size of the uncompressed data
            
        Returns:
            bytes: Decompressed data
            
        Raises:
            RuntimeError: If the library is not initialized or decompression fails
            ValueError: If arguments are invalid
        """
        if not self.initialized or self.decoder is NULL:
            raise RuntimeError("Library not initialized. Call initialize() first.")
        
        if not compressed_data:
            raise ValueError("No compressed data provided")
        
        if expected_size <= 0:
            raise ValueError("Expected size must be positive")
        
        cdef:
            XPRESS9_STATUS status
            unsigned int compressed_size = len(compressed_data)
            unsigned char* output_buffer = <unsigned char*>PyMem_Malloc(expected_size)
            unsigned int bytes_written = 0
            unsigned int bytes_needed = 0
            unsigned int remaining_bytes = 0
            const void* comp_data_ptr = <const void*>compressed_data
        
        if output_buffer is NULL:
            raise MemoryError("Failed to allocate memory for decompressed data")
        
        try:
            # Attach the compressed data
            Xpress9DecoderAttach(
                &status,
                self.decoder,
                comp_data_ptr,
                compressed_size
            )
            
            if status.m_uStatus != Xpress9Status_OK:
                error_msg = f"Failed to attach compressed data: {status.m_ErrorDescription.decode('utf-8')}"
                raise RuntimeError(error_msg)
            
            # Fetch decompressed data
            with nogil:
                remaining_bytes = Xpress9DecoderFetchDecompressedData(
                    &status,
                    self.decoder,
                    output_buffer,
                    expected_size,
                    &bytes_written,
                    &bytes_needed
                )
            
            # Check for errors
            if status.m_uStatus != Xpress9Status_OK:
                error_msg = f"Decompression failed: {status.m_ErrorDescription.decode('utf-8')}"
                raise RuntimeError(error_msg)
            
            # Detach the compressed data
            Xpress9DecoderDetach(
                &status,
                self.decoder,
                comp_data_ptr,
                compressed_size
            )
            
            if status.m_uStatus != Xpress9Status_OK:
                error_msg = f"Failed to detach compressed data: {status.m_ErrorDescription.decode('utf-8')}"
                raise RuntimeError(error_msg)
            
            # Verify we got the expected amount of data
            if bytes_written != expected_size:
                raise RuntimeError(
                    f"Expected {expected_size} bytes after decompression, "
                    f"but got {bytes_written} bytes"
                )
            
            # Create a Python bytes object from the decompressed data
            return PyBytes_FromStringAndSize(<char*>output_buffer, bytes_written)
        
        finally:
            # Always free the allocated memory
            PyMem_Free(output_buffer)