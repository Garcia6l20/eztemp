cmake_minimum_required(VERSION 3.0)

project(eztemp)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GenerateExportHeader)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)

find_package(Boost COMPONENTS date_time regex filesystem REQUIRED)

add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)

message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

if ("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    add_definitions(-DEZ_DEBUG=1)
endif()

set(src_files src/eztemp.cpp)
set(hdr_files_pub include/eztemp.h)
set(hdr_files_priv include/ezexpr.h)

add_library(${PROJECT_NAME} ${src_files} ${hdr_files_pub} ${hdr_files_priv})

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(FILES ${hdr_files_pub} DESTINATION include/${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC include ${Boost_INCLUDE_DIR} ${CMAKE_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}_export.h)

include_directories(${CMAKE_BINARY_DIR}/include)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

add_subdirectory(progs)
#add_subdirectory(bindings)
add_subdirectory(examples)

include(CTest)

set(CTEST_OUTPUT_ON_FAILURE TRUE)

add_subdirectory(tests)

set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 1)

include(CPack)
