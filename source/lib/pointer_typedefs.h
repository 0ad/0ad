#ifndef INCLUDED_POINTER_TYPEDEFS
#define INCLUDED_POINTER_TYPEDEFS

// convenience typedefs for shortening parameter lists.
// naming convention: [const] [restrict] pointer to [const] type
// supported types: void, signed/unsigned 8/16/32/64 integers, float, double, XMM

// NB: `restrict' is not going into C++0x, so use __restrict to maintain compatibility
// with VC2010.

typedef void* pVoid;
typedef void* const cpVoid;
typedef void* __restrict rpVoid;
typedef void* const __restrict crpVoid;
typedef const void* pcVoid;
typedef const void* const cpcVoid;
typedef const void* __restrict rpcVoid;
typedef const void* const __restrict crpcVoid;

typedef int8_t* pI8;
typedef int8_t* const cpI8;
typedef int8_t* __restrict rpI8;
typedef int8_t* const __restrict crpI8;
typedef const int8_t* pcI8;
typedef const int8_t* const cpcI8;
typedef const int8_t* __restrict rpcI8;
typedef const int8_t* const __restrict crpcI8;

typedef int16_t* pI16;
typedef int16_t* const cpI16;
typedef int16_t* __restrict rpI16;
typedef int16_t* const __restrict crpI16;
typedef const int16_t* pcI16;
typedef const int16_t* const cpcI16;
typedef const int16_t* __restrict rpcI16;
typedef const int16_t* const __restrict crpcI16;

typedef int32_t* pI32;
typedef int32_t* const cpI32;
typedef int32_t* __restrict rpI32;
typedef int32_t* const __restrict crpI32;
typedef const int32_t* pcI32;
typedef const int32_t* const cpcI32;
typedef const int32_t* __restrict rpcI32;
typedef const int32_t* const __restrict crpcI32;

typedef int64_t* pI64;
typedef int64_t* const cpI64;
typedef int64_t* __restrict rpI64;
typedef int64_t* const __restrict crpI64;
typedef const int64_t* pcI64;
typedef const int64_t* const cpcI64;
typedef const int64_t* __restrict rpcI64;
typedef const int64_t* const __restrict crpcI64;

typedef uint8_t* pU8;
typedef uint8_t* const cpU8;
typedef uint8_t* __restrict rpU8;
typedef uint8_t* const __restrict crpU8;
typedef const uint8_t* pcU8;
typedef const uint8_t* const cpcU8;
typedef const uint8_t* __restrict rpcU8;
typedef const uint8_t* const __restrict crpcU8;

typedef uint16_t* pU16;
typedef uint16_t* const cpU16;
typedef uint16_t* __restrict rpU16;
typedef uint16_t* const __restrict crpU16;
typedef const uint16_t* pcU16;
typedef const uint16_t* const cpcU16;
typedef const uint16_t* __restrict rpcU16;
typedef const uint16_t* const __restrict crpcU16;

typedef uint32_t* pU32;
typedef uint32_t* const cpU32;
typedef uint32_t* __restrict rpU32;
typedef uint32_t* const __restrict crpU32;
typedef const uint32_t* pcU32;
typedef const uint32_t* const cpcU32;
typedef const uint32_t* __restrict rpcU32;
typedef const uint32_t* const __restrict crpcU32;

typedef uint64_t* pU64;
typedef uint64_t* const cpU64;
typedef uint64_t* __restrict rpU64;
typedef uint64_t* const __restrict crpU64;
typedef const uint64_t* pcU64;
typedef const uint64_t* const cpcU64;
typedef const uint64_t* __restrict rpcU64;
typedef const uint64_t* const __restrict crpcU64;

typedef float* pFloat;
typedef float* const cpFloat;
typedef float* __restrict rpFloat;
typedef float* const __restrict crpFloat;
typedef const float* pcFloat;
typedef const float* const cpcFloat;
typedef const float* __restrict rpcFloat;
typedef const float* const __restrict crpcFloat;

typedef double* pDouble;
typedef double* const cpDouble;
typedef double* __restrict rpDouble;
typedef double* const __restrict crpDouble;
typedef const double* pcDouble;
typedef const double* const cpcDouble;
typedef const double* __restrict rpcDouble;
typedef const double* const __restrict crpcDouble;

typedef __m128* pM128;
typedef __m128* const cpM128;
typedef __m128* __restrict rpM128;
typedef __m128* const __restrict crpM128;
typedef const __m128* pcM128;
typedef const __m128* const cpcM128;
typedef const __m128* __restrict rpcM128;
typedef const __m128* const __restrict crpcM128;

typedef __m128i* pM128I;
typedef __m128i* const cpM128I;
typedef __m128i* __restrict rpM128I;
typedef __m128i* const __restrict crpM128I;
typedef const __m128i* pcM128I;
typedef const __m128i* const cpcM128I;
typedef const __m128i* __restrict rpcM128I;
typedef const __m128i* const __restrict crpcM128I;

typedef __m128d* pM128D;
typedef __m128d* const cpM128D;
typedef __m128d* __restrict rpM128D;
typedef __m128d* const __restrict crpM128D;
typedef const __m128d* pcM128D;
typedef const __m128d* const cpcM128D;
typedef const __m128d* __restrict rpcM128D;
typedef const __m128d* const __restrict crpcM128D;

typedef __m64* pM64;
typedef __m64* const cpM64;
typedef __m64* __restrict rpM64;
typedef __m64* const __restrict crpM64;
typedef const __m64* pcM64;
typedef const __m64* const cpcM64;
typedef const __m64* __restrict rpcM64;
typedef const __m64* const __restrict crpcM64;

#endif	// #ifndef INCLUDED_POINTER_TYPEDEFS
