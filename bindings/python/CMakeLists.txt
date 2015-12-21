cmake_minimum_required(VERSION 3.0)

project(pyztemp)

find_package(PythonLibs)
find_package(Boost COMPONENTS python3)

python_add_module(${PROJECT_NAME} src/pyztemp.cpp)

message(STATUS "Boost includes: ${Boost_INCLUDE_DIRS}")

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
