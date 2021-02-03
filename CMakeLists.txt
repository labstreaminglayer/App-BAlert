cmake_minimum_required(VERSION 3.5)

project(LSLBAlert
	DESCRIPTION "Transmit data from Advanced Brain Monitoring BAlert device over labstreaminglayer"
	HOMEPAGE_URL "https://github.com/labstreaminglayer/App-BAlert/"
	LANGUAGES C CXX
	VERSION 1.14.0)

find_package(LSL REQUIRED
	HINTS ${LSL_INSTALL_ROOT}
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/out/build/x86-Release"
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/out/install/x86-Release"
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/build/"
	"${CMAKE_CURRENT_LIST_DIR}/../../LSL/liblsl/build/install"
	PATH_SUFFIXES share/LSL
)

# Provide the path to the ABMAthena SDK with ABMAthena_ROOT variable. e.g. -DABMAthena_ROOT=C:\ABM_SDK\
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(ABMAthena REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} WIN32
	src/main.cpp
	src/mainwindow.cpp
	src/mainwindow.h
	src/mainwindow.ui
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	Qt5::Widgets
	Qt5::Network
	Threads::Threads
	LSL::lsl
	ABM::Athena
	ABM::BAlert
)

installLSLApp(${PROJECT_NAME})
installLSLAuxFiles(${PROJECT_NAME}
	${PROJECT_NAME}.cfg
	LICENSE
	README.md
)

if (WIN32)
	get_target_property(QT5_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(QT5_WINDEPLOYQT_EXECUTABLE ${QT5_QMAKE_EXECUTABLE} PATH)
	set(QT5_WINDEPLOYQT_EXECUTABLE "${QT5_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${QT5_WINDEPLOYQT_EXECUTABLE} --qmldir
			${CMAKE_CURRENT_SOURCE_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different  
			$<TARGET_FILE:LSL::lsl>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>)
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.cfg
			$<TARGET_FILE_DIR:${PROJECT_NAME}>)

	# Copy dlls
	# ABM_Athena.dll, BAlert.dll, ABM_ThirdPartyCommunication.dll, ABM_Datastreaming.dll, SiUSBXp.dll
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			$<TARGET_FILE_DIR:ABM::Athena>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>)
	
endif()

LSLGenerateCPackConfig()
