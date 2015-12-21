cmake_minimum_required(VERSION 3.0)

project(helloworld)

add_executable(${PROJECT_NAME}
    helloworld.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE eztemp)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

