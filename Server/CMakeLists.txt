# CMakeList.txt : CMake project for FourNetworking, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.16)
project(SampleServer)

set(CMAKE_CXX_STANDARD 20)

include_directories("src")

file(GLOB SOURCES main.cpp src/* src/*/* src/*/*)

find_package(Boost)

# Add source to this project's executable.
add_executable (${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} 
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		${Boost_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
	PUBLIC
		${Boost_LIBRARIES}
)