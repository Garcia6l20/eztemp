cmake_minimum_required(VERSION 3.0)

project(eztemp-cc)

add_executable(${PROJECT_NAME}
    src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE eztemp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_uniform_initialization)

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
