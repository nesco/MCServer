cmake_minimum_required (VERSION 2.8.2)

# Without this, the MSVC variable isn't defined for MSVC builds ( http://www.cmake.org/pipermail/cmake/2011-November/047130.html )
enable_language(CXX C)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../" ${MAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../lib/cmake-coverage/" ${CMAKE_MODULE_PATH} )

include(SetFlags)
set_flags()
enable_profile()
set_exe_flags()

include_directories(../../lib)
include_directories(../../src)

add_executable(LightingBenchmark BenchMark LightingThreadOld.cpp LightingThreadHeightMap.cpp BlockInfo.cpp ../../src/ChunkData.cpp ../../src/Noise.cpp ../../src/StringUtils.cpp ../../src/OSSupport/File.cpp)
