find_package(SDL3 CONFIG REQUIRED)

add_executable(basic basic.c)
target_include_directories(basic PUBLIC ${SDL3_INCLUDE_DIRS})
target_link_libraries(basic PUBLIC ${SDL3_LIBRARIES})
