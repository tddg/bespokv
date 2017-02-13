cmake_minimum_required(VERSION 2.6)
project(client)
set(CMAKE_MACOSX_RPATH 1)

add_definitions(-DMC_USE_SMALL_VECTOR)

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_COMMON "-std=c++11 -Wall -fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g2 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX DEBUG FLAGS" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX RELEASE FLAGS" FORCE)
set(CMAKE_INSTALL_INCLUDE include CACHE PATH "Output directory for header files")
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

include_directories(../include)
#link_directories(${client_SOURCE_DIR})
link_directories(/root/ClusterOn/conproxy/apps/clibs/libckv/build)
#link_directories(/root/ClusterOn/conproxy/apps/clibs/libckv/test)

set(lib ckv)
find_package(Protobuf REQUIRED)

FILE(
    GLOB client_HEADERS
    *.h
    ../include/*.h
    ../include/hashkit/*.h
    ../include/llvm/*.h
    ../include/rapidjson/*.h
)
FILE(
    GLOB client_SOURCES
    ./*.cpp
    ./*.cc
)
add_executable(client ${client_SOURCES})
#link_directories(/root/ClusterOn/conproxy/src/apps/ckv/libckv/build)
target_link_libraries(client
	${lib}
	${PROTOBUF_LIBRARY})

#add_library(ckv STATIC ${ckv_SOURCES})
#
#install(
#    FILES include/c_client.h
#    DESTINATION ${CMAKE_INSTALL_INCLUDE}/${CMAKE_PROJECT}
#)
#install(
#    TARGETS ckv
#    DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    EXPORT ckv
#)

# option(WITH_TESTING "Enable testing" OFF)
# if (WITH_TESTING)
#     set(EXT_PROJECTS_DIR ${CMAKE_SOURCE_DIR}/ext)
#     add_subdirectory(${EXT_PROJECTS_DIR}/gtest)
#     enable_testing()
#     add_subdirectory(tests)
# endif (WITH_TESTING)
