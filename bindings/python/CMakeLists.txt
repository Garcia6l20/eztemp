cmake_minimum_required(VERSION 3.0)

project(pyztemp)

find_package(PythonLibs 3)
find_package(Boost COMPONENTS python REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )

set(PYZTEMP_USES_DISTUTILS "build pyztemp python extension using distutils rather than CMake." ON)

message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")

if(NOT PYZTEMP_USES_DISTUTILS)

    python_add_module(${PROJECT_NAME} src/pyztemp.cpp)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

else()

    function(JOIN VALUES GLUE OUTPUT)
      string (REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
      string (REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
      set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
    endfunction()

    join("${Boost_INCLUDE_DIRS};${PYTHON_INCLUDE_DIRS}" "\", \"" SETUP_INCLUDE_DIRS)
    set(SETUP_INCLUDE_DIRS "\"${SETUP_INCLUDE_DIRS}\"")

    join("${Boost_LIBRARIES};${PYTHON_LIBRARIES}" "\", \"" SETUP_LIBRARIES)
    set(SETUP_LIBRARIES "\"${SETUP_LIBRARIES}\"")

    set(SETUP_FILES "\"${PROJECT_SOURCE_DIR}/src/pyztemp.cpp\"")

    configure_file(setup.py.in setup.py)

    add_custom_target(${PROJECT_NAME} ALL COMMAND python3 ${PROJECT_BINARY_DIR}/setup.py build
                                          COMMAND cp ${PROJECT_BINARY_DIR}/build/**/${PROJECT_NAME}*.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

endif()
