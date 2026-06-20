cmake_minimum_required(VERSION 3.5)
project(sdl3fuzz)
add_executable(sdl3-fuzz src/sdl3fuzz.c)
add_library(sdl3fuzz SHARED src/SDL_fuzz.c)

find_package(SDL3 CONFIG REQUIRED)
target_include_directories(sdl3fuzz PUBLIC ${SDL3_INCLUDE_DIRS})
target_link_libraries(sdl3fuzz PUBLIC ${SDL3_LIBRARIES})
target_include_directories(sdl3-fuzz PUBLIC ${SDL3_INCLUDE_DIRS})
target_link_libraries(sdl3-fuzz PUBLIC ${SDL3_LIBRARIES})

target_compile_definitions(sdl3-fuzz PUBLIC -DLIB_PATH=\"$<TARGET_FILE:sdl3fuzz>\")

option(SDL3FUZZ_BUILD_EXAMPLES "Build example programs" OFF)
if(SDL3FUZZ_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
