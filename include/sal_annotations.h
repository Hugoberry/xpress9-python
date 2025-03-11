#pragma once

// Define SAL annotations if not already defined
// These are Microsoft-specific annotations used for static analysis
#ifndef _SAL_Annotations
#define _SAL_Annotations

#ifdef _MSC_VER
    // Already defined in Microsoft compiler
#else
    // Definitions for non-Microsoft compilers
    #define __in
    #define __out
    #define __in_bcount(size)
    #define __out_bcount(size)
    #define __in_ecount(size)
    #define __out_ecount(size)
    #define __in_opt
    #define __out_opt
    #define __inout
    #define __inout_opt
    #define __in_bcount_opt(size)
    #define __out_bcount_opt(size)
    #define __in_ecount_opt(size)
    #define __out_ecount_opt(size)
    #define __in_z
    #define __out_z
    #define __deref_out
    #define __deref_out_opt
    #define __deref_opt_out
    #define __analysis_assume(expr)
#endif

#endif // _SAL_Annotations