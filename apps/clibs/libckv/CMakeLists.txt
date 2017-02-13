cmake_minimum_required(VERSION 2.6)
project(libckv)
set(CMAKE_MACOSX_RPATH 1)

set (CKV_VERSION_MAJOR 1)
set (CKV_VERSION_MINOR 0)
#set (CKV_VERSION_PATCH 1)

set (CKV_VERSION ${CKV_VERSION_MAJOR}.${CKV_VERSION_MINOR})
#set (MC_APIVERSION ${MC_VERSION}.${MC_VERSION_PATCH})

add_definitions(-DMC_USE_SMALL_VECTOR)

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_COMMON "-std=c++11 -Wall -fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g2 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX DEBUG FLAGS" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX RELEASE FLAGS" FORCE)
set(CMAKE_INSTALL_INCLUDE include CACHE PATH "Output directory for header files")
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

include_directories(include)
#set(libs protobuf)
find_package(Protobuf REQUIRED)

FILE(
    GLOB ckv_HEADERS
    ./include/*.h
    ./include/hashkit/*.h
    ./include/llvm/*.h
    ./include/rapidjson/*.h
)
FILE(
    GLOB ckv_SOURCES
    ./*.cpp
    ./*.cc
)
add_library(ckv STATIC ${ckv_SOURCES})
target_link_libraries(ckv 
	${libs}
	${PROTOBUF_LIBRARY})

install(
    FILES include/c_client.h
    DESTINATION ${CMAKE_INSTALL_INCLUDE}/${CMAKE_PROJECT}
)
install(
    TARGETS ckv
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    EXPORT ckv
)

# option(WITH_TESTING "Enable testing" OFF)
# if (WITH_TESTING)
#     set(EXT_PROJECTS_DIR ${CMAKE_SOURCE_DIR}/ext)
#     add_subdirectory(${EXT_PROJECTS_DIR}/gtest)
#     enable_testing()
#     add_subdirectory(tests)
# endif (WITH_TESTING)
