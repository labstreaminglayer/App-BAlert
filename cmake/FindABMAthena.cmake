# find_package module for ABMAthena SDK
# Usage: typically this file will be in the `./cmake` folder of your project.
# 	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
#	find_package(ABMAthena REQUIRED)
#	ABMAthena_INCLUDE_DIRS
#	ABMAthena_LIBRARIES

cmake_minimum_required(VERSION 2.6.3)
include(FindPackageHandleStandardArgs)

set(ABMAthena_ROOT_DIR
	"${ABMAthena_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search for ABMAthena SDK"
)

find_path(ABMAthena_INCLUDE_DIR
	NAMES
		AbmSdkInclude.h
	PATHS
		${ABMAthena_ROOT_DIR}
	PATH_SUFFIXES
		include
)

find_library(ABMAthena_LIBRARY
	NAMES
		ABM_Athena
	PATHS
		${ABMAthena_ROOT_DIR}
	PATH_SUFFIXES
		lib
)
if(WIN32 AND ABMAthena_LIBRARY)
	find_file(ABMAthena_RUNTIME_LIBRARY
		NAMES
			ABM_Athena.dll
		PATHS
			${ABMAthena_ROOT_DIR}
		PATH_SUFFIXES
			bin lib
	)
endif()


find_library(BAlert_LIBRARY
	NAMES
		BAlert
	PATHS
		${ABMAthena_ROOT_DIR}
	PATH_SUFFIXES
		lib
)
if(WIN32 AND BAlert_LIBRARY)
	find_file(BAlert_RUNTIME_LIBRARY
		NAMES
			BAlert.dll
		PATHS
			${ABMAthena_ROOT_DIR}
		PATH_SUFFIXES
			bin lib
	)
endif()

find_package_handle_standard_args(ABMAthena
	DEFAULT_MSG	
	ABMAthena_LIBRARY
	ABMAthena_INCLUDE_DIR
)

if(ABMAthena_FOUND)
	if(NOT TARGET ABM::Athena)
		if(WIN32 AND ABMAthena_RUNTIME_LIBRARY)
			add_library(ABM::Athena SHARED IMPORTED)
			set_target_properties(ABM::Athena
				PROPERTIES
					IMPORTED_IMPLIB "${ABMAthena_LIBRARY}"
					IMPORTED_LOCATION "${ABMAthena_RUNTIME_LIBRARY}"
					INTERFACE_INCLUDE_DIRECTORIES "${ABMAthena_INCLUDE_DIR}"
					IMPORTED_LINK_INTERFACE_LANGUAGES "C"
			)
		endif()
	endif()
	if(NOT TARGET ABM::BAlert)
		if(WIN32 AND BAlert_RUNTIME_LIBRARY)
			add_library(ABM::BAlert SHARED IMPORTED)
			set_target_properties(ABM::BAlert
				PROPERTIES
					IMPORTED_IMPLIB "${BAlert_LIBRARY}"
					IMPORTED_LOCATION "${BAlert_RUNTIME_LIBRARY}"
					INTERFACE_INCLUDE_DIRECTORIES "${ABMAthena_INCLUDE_DIR}"
					IMPORTED_LINK_INTERFACE_LANGUAGES "C"
			)
		endif()
	endif()
endif()

mark_as_advanced(
	ABMAthena_LIBRARY
	ABMAthena_RUNTIME_LIBRARY
	ABMAthena_INCLUDE_DIR
)
