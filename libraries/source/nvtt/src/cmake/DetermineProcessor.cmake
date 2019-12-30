
# Assume i586 by default.
SET(NV_SYSTEM_PROCESSOR "i586")

IF(UNIX)
	FIND_PROGRAM(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin )
	IF(CMAKE_UNAME)
		#EXEC_PROGRAM(uname ARGS -p OUTPUT_VARIABLE NV_SYSTEM_PROCESSOR RETURN_VALUE val)

		#IF("${val}" GREATER 0 OR NV_SYSTEM_PROCESSOR STREQUAL "unknown")
			EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE NV_SYSTEM_PROCESSOR RETURN_VALUE val)
		#ENDIF("${val}" GREATER 0 OR NV_SYSTEM_PROCESSOR STREQUAL "unknown")

		IF(NV_SYSTEM_PROCESSOR STREQUAL "Power Macintosh")
			SET(NV_SYSTEM_PROCESSOR "powerpc")
		ENDIF(NV_SYSTEM_PROCESSOR STREQUAL "Power Macintosh")

		# processor may have double quote in the name, and that needs to be removed
		STRING(REGEX REPLACE "\"" "" NV_SYSTEM_PROCESSOR "${NV_SYSTEM_PROCESSOR}")
		STRING(REGEX REPLACE "/" "_" NV_SYSTEM_PROCESSOR "${NV_SYSTEM_PROCESSOR}")
	ENDIF(CMAKE_UNAME)

#~ 	# Get extended processor information from /proc/cpuinfo
#~ 	IF(EXISTS "/proc/cpuinfo")

#~ 		FILE(READ /proc/cpuinfo PROC_CPUINFO)

#~ 		SET(VENDOR_ID_RX "vendor_id[ \t]*:[ \t]*([a-zA-Z]+)\n")
#~ 		STRING(REGEX MATCH "${VENDOR_ID_RX}" VENDOR_ID "${PROC_CPUINFO}")
#~ 		STRING(REGEX REPLACE "${VENDOR_ID_RX}" "\\1" VENDOR_ID "${VENDOR_ID}")

#~ 		SET(CPU_FAMILY_RX "cpu family[ \t]*:[ \t]*([0-9]+)")
#~ 		STRING(REGEX MATCH "${CPU_FAMILY_RX}" CPU_FAMILY "${PROC_CPUINFO}")
#~ 		STRING(REGEX REPLACE "${CPU_FAMILY_RX}" "\\1" CPU_FAMILY "${CPU_FAMILY}")

#~ 		SET(MODEL_RX "model[ \t]*:[ \t]*([0-9]+)")
#~ 		STRING(REGEX MATCH "${MODEL_RX}" MODEL "${PROC_CPUINFO}")
#~ 		STRING(REGEX REPLACE "${MODEL_RX}" "\\1" MODEL "${MODEL}")

#~ 		SET(FLAGS_RX "flags[ \t]*:[ \t]*([a-zA-Z0-9 _]+)\n")
#~ 		STRING(REGEX MATCH "${FLAGS_RX}" FLAGS "${PROC_CPUINFO}")
#~ 		STRING(REGEX REPLACE "${FLAGS_RX}" "\\1" FLAGS "${FLAGS}")

#~ 		# Debug output.
#~ 		IF(LINUX_CPUINFO)
#~ 			MESSAGE(STATUS "LinuxCPUInfo.cmake:")
#~ 			MESSAGE(STATUS "VENDOR_ID : ${VENDOR_ID}")
#~ 			MESSAGE(STATUS "CPU_FAMILY : ${CPU_FAMILY}")
#~ 			MESSAGE(STATUS "MODEL : ${MODEL}")
#~ 			MESSAGE(STATUS "FLAGS : ${FLAGS}")
#~ 		ENDIF(LINUX_CPUINFO)

#~ 	ENDIF(EXISTS "/proc/cpuinfo")

#~		# Information on how to decode CPU_FAMILY and MODEL:
#~		# http://balusc.xs4all.nl/srv/har-cpu-int-pm.php

ELSE(UNIX)

  IF(WIN32)
    # It's not OK to trust $ENV{PROCESSOR_ARCHITECTURE}: its value depends on the type of executable being run,
	# so a 32-bit cmake (the default binary distribution) will always say "x86" regardless of the actual target.
	IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
      SET (NV_SYSTEM_PROCESSOR "x86_64")
	ELSE(CMAKE_SIZEOF_VOID_P EQUAL 8)
	  SET (NV_SYSTEM_PROCESSOR "x86")
	ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  ENDIF(WIN32)

ENDIF(UNIX)


