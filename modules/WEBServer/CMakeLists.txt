cmake_minimum_required(VERSION 2.6)

SET(TARGET WEBServer)
	
PROJECT(${TARGET})

CREATE_MODULE(SRCS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

SET(SRCS ${SRCS}
	stdafx.cpp
	"${TARGET}.cpp"
	"handler_impl.cpp"
	${NSCP_INCLUDEDIR}/nrpe/packet.cpp
	${NSCP_INCLUDEDIR}/socket/socket_helpers.cpp

	${NSCP_DEF_PLUGIN_CPP}
)

ADD_DEFINITIONS(${NSCP_GLOBAL_DEFINES})
IF(OPENSSL_FOUND)
	INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
	ADD_DEFINITIONS(-DUSE_SSL)
	SET(EXTRA_LIBS ${EXTRA_LIBS} ${OPENSSL_LIBRARIES})
ENDIF(OPENSSL_FOUND)
IF(MONGOOSE_FOUND)
	INCLUDE_DIRECTORIES(${MONGOOSE_INCLUDE_DIR})
	SET(EXTRA_LIBS ${EXTRA_LIBS} ${MONGOOSE_LIBRARY})
ENDIF(MONGOOSE_FOUND)

IF(WIN32)
	SET(SRCS ${SRCS}
		stdafx.h
		"${TARGET}.h"
		"handler_impl.hpp"
		${NSCP_INCLUDEDIR}/nrpe/server/protocol.hpp
		${NSCP_INCLUDEDIR}/nrpe/server/handler.hpp
		${NSCP_INCLUDEDIR}/nrpe/server/parser.hpp
		${NSCP_INCLUDEDIR}/nrpe/packet.hpp
		${NSCP_INCLUDEDIR}/swap_bytes.hpp
		${NSCP_INCLUDEDIR}/socket/socket_helpers.hpp
		${NSCP_INCLUDEDIR}/socket/server.hpp
		${NSCP_INCLUDEDIR}/socket/connection.hpp

		${NSCP_DEF_PLUGIN_HPP}
	)
ENDIF(WIN32)

add_library(${TARGET} MODULE ${SRCS})
OPENSSL_LINK_FIX(${TARGET})

target_link_libraries(${TARGET}
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_THREAD_LIBRARY}
	${NSCP_DEF_PLUGIN_LIB}
	${EXTRA_LIBS}
)
INCLUDE(${BUILD_CMAKE_FOLDER}/module.cmake)
SOURCE_GROUP("Server" REGULAR_EXPRESSION .*include/nrpe/.*)
SOURCE_GROUP("Socket" REGULAR_EXPRESSION .*include/socket/.*)
