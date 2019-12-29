#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_PATH
# GLEW_LIBRARY
# 

IF (WIN32)
	FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
		$ENV{PROGRAMFILES}/GLEW/include
		${GLEW_ROOT_DIR}/include
		DOC "The directory where GL/glew.h resides")

    FIND_LIBRARY( GLEW_LIBRARY
        NAMES glew GLEW glew32 glew32s
        PATHS
        $ENV{PROGRAMFILES}/GLEW/lib
        ${PROJECT_SOURCE_DIR}/src/nvgl/glew/bin
        ${PROJECT_SOURCE_DIR}/src/nvgl/glew/lib
        DOC "The GLEW library")
ELSE (WIN32)
	FIND_PATH( GLEW_INCLUDE_PATH GL/glew.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		${GLEW_ROOT_DIR}/include
		DOC "The directory where GL/glew.h resides")

	# Prefer the static library.
	FIND_LIBRARY( GLEW_LIBRARY
		NAMES libGLEW.a GLEW
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		${GLEW_ROOT_DIR}/lib
		DOC "The GLEW library")
ENDIF (WIN32)

SET(GLEW_FOUND "NO")
IF (GLEW_INCLUDE_PATH AND GLEW_LIBRARY)
	SET(GLEW_LIBRARIES ${GLEW_LIBRARY})
	SET(GLEW_FOUND "YES")
ENDIF (GLEW_INCLUDE_PATH AND GLEW_LIBRARY)
