cmake_minimum_required(VERSION 3.8)

project(ORNG_RUNTIME)

add_executable(ORNG_RUNTIME
src/RuntimeLayer.cpp
 "src/main.cpp")


target_include_directories(ORNG_RUNTIME PUBLIC
${ORNG_CORE_INCLUDE_DIRS}
headers
)

target_link_libraries(ORNG_RUNTIME PUBLIC 
ORNG_CORE
imgui
)

target_precompile_headers(ORNG_RUNTIME REUSE_FROM ORNG_CORE)


add_custom_command(TARGET ORNG_RUNTIME POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:ORNG_RUNTIME>)
foreach(core_binary IN LISTS ORNG_CORE_BINARIES)
    add_custom_command(TARGET ORNG_RUNTIME POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${core_binary}
        $<TARGET_FILE_DIR:ORNG_RUNTIME>)
endforeach()
