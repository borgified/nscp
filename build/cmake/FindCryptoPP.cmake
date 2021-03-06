# -*- cmake -*-
#
# Find crypto++ library or sources.
#
# Input variables:
#   CRYPTOPP_DIR	- set this to specify the crypto++ source to be built.
#
# Output variables:
#
#   CRYPTOPP_FOUND			- set if library was found
#	CRYPTOPP_INCLUDE_DIR	- Set to where include files ar found (or sources)
#	CRYPTOPP_LIBRARIES		- Set to library

FIND_PATH(CRYPTOPP_INCLUDE_DIR 
	cryptlib.h
	PATHS
		${CRYPTOPP_DIR}
		/usr/include/crypto++
		/usr/include/cryptopp
		/usr/include
)

SET(CRYPTOPP_LIB_ROOT)
if(CMAKE_CL_64)
	SET(CRYPTOPP_LIB_ROOT ${CRYPTOPP_INCLUDE_DIR}/x64/output)
ELSE()
	SET(CRYPTOPP_LIB_ROOT ${CRYPTOPP_INCLUDE_DIR}/Win32/output)
ENDIF()

FIND_LIBRARY(CRYPTOPP_LIBRARIES_RELEASE
	NAMES crypto++ cryptlib cryptopp
	PATHS
		${CRYPTOPP_LIB_ROOT}/release
		${CRYPTOPP_LIB_ROOT}
		/usr/lib/
)
FIND_LIBRARY(CRYPTOPP_LIBRARIES_DEBUG
	NAMES crypto++ cryptlib cryptopp
	PATHS
		${CRYPTOPP_LIB_ROOT}/debug
		${CRYPTOPP_LIB_ROOT}
		/usr/lib/
)


IF(NOT CRYPTOPP_INCLUDE_DIR OR NOT CRYPTOPP_LIBRARIES_RELEASE)
	FIND_PATH(CRYPTOPP_CMAKE 
		NAMES CMakeLists.txt
		PATHS
			${CMAKE_SOURCE_DIR}/ext/cryptopp
			${CRYPTOPP_DIR}/
			${NSCP_INCLUDEDIR}
			)


	IF(CRYPTOPP_CMAKE)
		FIND_PATH(CRYPTOPP_INCLUDE_DIR 
			cryptlib.h
			PATHS
				${CRYPTOPP_CMAKE}/src
		)
	ENDIF()
ENDIF()

IF(CMAKE_TRACE)
	MESSAGE(STATUS " - CRYPTOPP_INCLUDE_DIR=${CRYPTOPP_INCLUDE_DIR}")
	MESSAGE(STATUS " - CRYPTOPP_LIBRARIES_DEBUG=${CRYPTOPP_LIBRARIES_DEBUG}")
	MESSAGE(STATUS " - CRYPTOPP_LIBRARIES_RELEASE=${CRYPTOPP_LIBRARIES_RELEASE}")
	MESSAGE(STATUS " - CRYPTOPP_DIR=${CRYPTOPP_DIR}")
	MESSAGE(STATUS " - CRYPTOPP_CMAKE=${CRYPTOPP_CMAKE}")
ENDIF()

IF(CRYPTOPP_CMAKE)
	IF(CRYPTOPP_INCLUDE_DIR)
		SET(CRYPTOPP_FOUND TRUE)
		SET(CRYPTOPP_LIBRARIES cryptlib)
	ENDIF()
ELSE()
	IF(CRYPTOPP_INCLUDE_DIR AND CRYPTOPP_LIBRARIES_RELEASE AND CRYPTOPP_LIBRARIES_DEBUG)
		SET(CRYPTOPP_FOUND TRUE)
		SET(CRYPTOPP_LIBRARIES optimized ${CRYPTOPP_LIBRARIES_RELEASE} debug ${CRYPTOPP_LIBRARIES_DEBUG})
	ENDIF()
ENDIF()
IF(CMAKE_TRACE)
	MESSAGE(STATUS " - CRYPTOPP_LIBRARIES=${CRYPTOPP_LIBRARIES}")
ENDIF()
