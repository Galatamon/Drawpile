set (QT_DONT_USE_QTGUI true)
include ( ${QT_USE_FILE} )

add_definitions(${QT_DEFINITIONS})

set (
	SOURCES
	dpserver.cpp
	)

#QT4_WRAP_CPP (MOC_Sources ${SRV_QHEADERS} )

if (NOT DEBUG)
	add_definitions(-DQT_NO_DEBUG)
endif ( NOT DEBUG)

link_libraries( ${DPSHAREDLIB} ${QT_LIBRARIES} )
add_executable( ${SRVNAME} ${SOURCES} )

if ( WIN32 )
	install ( TARGETS ${SRVNAME} DESTINATION . )
else ( WIN32 )
	install ( TARGETS ${SRVNAME} DESTINATION bin )
endif ( WIN32 )

