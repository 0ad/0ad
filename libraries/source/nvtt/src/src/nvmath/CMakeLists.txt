PROJECT(nvmath)

SET(MATH_SRCS
    nvmath.h
    Box.h Box.inl
    Color.h Color.inl
    Fitting.h Fitting.cpp
    Gamma.h Gamma.cpp
    Half.h Half.cpp
    Matrix.h
    Plane.h Plane.inl Plane.cpp
    SphericalHarmonic.h SphericalHarmonic.cpp
    SimdVector.h SimdVector_SSE.h SimdVector_VE.h
    Vector.h Vector.inl)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# targets
ADD_DEFINITIONS(-DNVMATH_EXPORTS)

IF(NVMATH_SHARED)
    ADD_DEFINITIONS(-DNVMATH_SHARED=1)
    ADD_LIBRARY(nvmath SHARED ${MATH_SRCS})
ELSE(NVMATH_SHARED)
    ADD_LIBRARY(nvmath ${MATH_SRCS})
ENDIF(NVMATH_SHARED)

TARGET_LINK_LIBRARIES(nvmath ${LIBS} nvcore)

INSTALL(TARGETS nvmath
    RUNTIME DESTINATION ${BINDIR}
    LIBRARY DESTINATION ${LIBDIR}
    ARCHIVE DESTINATION ${LIBDIR})
