cmake_minimum_required(VERSION 3.5)
project(sdl3fuzz)
add_library(sdl3fuzz SHARED src/SDL_fuzz.c)

find_package(SDL3 CONFIG REQUIRED)
target_include_directories(sdl3fuzz PUBLIC ${SDL3_INCLUDE_DIRS})
target_link_libraries(sdl3fuzz PUBLIC ${SDL3_LIBRARIES})
