cmake_minimum_required (VERSION 2.8.7)

# Without this, the MSVC variable isn't defined for MSVC builds ( http://www.cmake.org/pipermail/cmake/2011-November/047130.html )
enable_language(CXX C)

# These env variables are used for configuring Travis CI builds.
# See https://github.com/mc-server/MCServer/pull/767
if(DEFINED ENV{TRAVIS_MCSERVER_BUILD_TYPE})
	message("Setting build type to $ENV{TRAVIS_MCSERVER_BUILD_TYPE}")
	set(CMAKE_BUILD_TYPE $ENV{TRAVIS_MCSERVER_BUILD_TYPE})
endif()

if(DEFINED ENV{TRAVIS_MCSERVER_FORCE32})
	set(FORCE32 $ENV{TRAVIS_MCSERVER_FORCE32})
endif()

if(DEFINED ENV{TRAVIS_BUILD_WITH_COVERAGE})
	set(BUILD_WITH_COVERAGE $ENV{TRAVIS_BUILD_WITH_COVERAGE})
endif()

if(DEFINED ENV{MCSERVER_BUILD_ID})
	set(BUILD_ID $ENV{MCSERVER_BUILD_ID})
	set(BUILD_SERIES_NAME $ENV{MCSERVER_BUILD_SERIES_NAME})
	set(BUILD_DATETIME $ENV{MCSERVER_BUILD_DATETIME})
	if(DEFINED ENV{MCSERVER_BUILD_COMMIT_ID})
		set(BUILD_COMMIT_ID $ENV{MCSERVER_BUILD_COMMIT_ID})
	else()
		message("Commit id not set, attempting to determine id from git")
		execute_process(
			COMMAND git rev-parse HEAD
			RESULT_VARIABLE GIT_EXECUTED
			OUTPUT_VARIABLE BUILD_COMMIT_ID)
			string(STRIP ${BUILD_COMMIT_ID} BUILD_COMMIT_ID)
		if (NOT (GIT_EXECUTED EQUAL 0))
			message(FATAL_ERROR "Could not identifiy git commit id")
		endif()
	endif()
endif()

# This has to be done before any flags have been set up.
if(${BUILD_TOOLS})
	add_subdirectory(Tools/MCADefrag/)
	add_subdirectory(Tools/ProtoProxy/)
endif()

if(${BUILD_UNSTABLE_TOOLS})
	add_subdirectory(Tools/GeneratorPerformanceTest/)
endif()

include(SetFlags.cmake)
set_flags()
set_lib_flags()
enable_profile()

# Under Windows, we need Lua as DLL; on *nix we need it linked statically:
if (WIN32)
	add_definitions(-DLUA_BUILD_AS_DLL)
endif()


# The Expat library is linked in statically, make the source files aware of that:
add_definitions(-DXML_STATIC)

# Self Test Mode enables extra checks at startup
if(${SELF_TEST})
	add_definitions(-DSELF_TEST)
endif()





project (MCServer)

# Set options for SQLiteCpp, disable all their tests and lints:
set(SQLITECPP_RUN_CPPLINT     OFF CACHE BOOL "Run cpplint.py tool for Google C++ StyleGuide."  FORCE)
set(SQLITECPP_RUN_CPPCHECK    OFF CACHE BOOL "Run cppcheck C++ static analysis tool."          FORCE)
set(SQLITECPP_RUN_DOXYGEN     OFF CACHE BOOL "Run Doxygen C++ documentation tool."             FORCE)
set(SQLITECPP_BUILD_EXAMPLES  OFF CACHE BOOL "Build examples."                                 FORCE)
set(SQLITECPP_BUILD_TESTS     OFF CACHE BOOL "Build and run tests."                            FORCE)
set(SQLITECPP_INTERNAL_SQLITE OFF CACHE BOOL "Add the internal SQLite3 source to the project." FORCE)

# Include all the libraries:
add_subdirectory(lib/inifile/)
add_subdirectory(lib/jsoncpp/)
add_subdirectory(lib/zlib/)
add_subdirectory(lib/lua/)
add_subdirectory(lib/tolua++/)
add_subdirectory(lib/sqlite/)
add_subdirectory(lib/SQLiteCpp/)
add_subdirectory(lib/expat/)
add_subdirectory(lib/luaexpat/)

# Add proper include directories so that SQLiteCpp can find SQLite3:
get_property(SQLITECPP_INCLUDES DIRECTORY "lib/SQLiteCpp/" PROPERTY INCLUDE_DIRECTORIES)
set(SQLITECPP_INCLUDES "${SQLITECPP_INCLUDES}" "${CMAKE_CURRENT_SOURCE_DIR}/lib/sqlite/")
set_property(DIRECTORY lib/SQLiteCpp/ PROPERTY INCLUDE_DIRECTORIES "${SQLITECPP_INCLUDES}")
set_property(TARGET SQLiteCpp PROPERTY INCLUDE_DIRECTORIES "${SQLITECPP_INCLUDES}")

if (WIN32)
	add_subdirectory(lib/luaproxy/)
endif()


# We use EXCLUDE_FROM_ALL so that only the explicit dependencies are used
# (PolarSSL also has test and example programs in their CMakeLists.txt, we don't want those)
include(lib/polarssl.cmake)

set_exe_flags()

add_subdirectory (src)

if(${SELF_TEST})
	enable_testing()
	add_subdirectory (tests)
endif()

