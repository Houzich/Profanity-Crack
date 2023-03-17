#ifndef HPP_TYPES
#define HPP_TYPES

/* The structs declared in this file should have size/alignment hints
 * to ensure that their representation is identical to that in OpenCL.
 */
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include "defines.h"


#define MP_NWORDS 8

typedef cl_uint mp_word;

typedef struct {
	mp_word d[MP_NWORDS];
} mp_number;

typedef struct {
    mp_number x;
    mp_number y;
} point;

typedef struct {
	cl_ulong8 key;
} public_key;

typedef struct {
	cl_ulong4 key;
} private_key;

typedef struct {
	cl_ulong round;
	cl_uint found;
	cl_uint foundId;
	mp_number addr;
	public_key pub_key;
} result;

#endif /* HPP_TYPES */