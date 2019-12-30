PROJECT(bc7)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

SET(BC7_SRCS
	avpcl.cpp
	avpcl.h
	avpcl_mode0.cpp
	avpcl_mode1.cpp
	avpcl_mode2.cpp
	avpcl_mode3.cpp
	avpcl_mode4.cpp
	avpcl_mode5.cpp
	avpcl_mode6.cpp
	avpcl_mode7.cpp
	bits.h
	endpts.h
	shapes_three.h
	shapes_two.h
	tile.h
	avpcl_utils.cpp
	avpcl_utils.h)

ADD_LIBRARY(bc7 STATIC ${BC7_SRCS})

IF(NOT WIN32)
    IF(CMAKE_COMPILER_IS_GNUCXX)
        SET_TARGET_PROPERTIES(bc7 PROPERTIES COMPILE_FLAGS -fPIC)
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDIF(NOT WIN32)
