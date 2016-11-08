cmake_minimum_required(VERSION 3.0)

project(eztemp-tests)

# command line test

add_test(NAME basic_test COMMAND ${CMAKE_BINARY_DIR}/bin/eztemp-cc "Hello {{ name }} !\n" -p "{ \"name\" : \"6L20\" }" WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME date_test COMMAND ${CMAKE_BINARY_DIR}/bin/eztemp-cc "Today it's {{ date() }} !\n" WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME for_test COMMAND ${CMAKE_BINARY_DIR}/bin/eztemp-cc "{% for item in table %}-> {{ item }}\n{% endfor %}\n" -p "{ \"table\" : [1, 2, 3] }" WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_test(NAME index_test COMMAND ${CMAKE_BINARY_DIR}/bin/eztemp-cc "templates/index.html.ez" -p "{ \"who\" : \"world\" }" > "index.html" WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_custom_target(${PROJECT_NAME} COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

add_custom_target(${PROJECT_NAME}-templates ALL ${CMAKE_COMMAND} -E copy_directory
                        ${PROJECT_SOURCE_DIR}/templates ${CMAKE_BINARY_DIR}/bin/templates
                        DEPENDS templates/layout.html.ez templates/index.html.ez)

add_dependencies(${PROJECT_NAME} eztemp-cc)
