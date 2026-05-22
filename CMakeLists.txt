cmake_minimum_required(VERSION 3.15)
project(kinnie VERSION 2.3.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(kinnie
    src/main.c
    src/lexer.c
    src/parser.c
    src/codegen.c
)

target_include_directories(kinnie PRIVATE include)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET sdl2)
    if(SDL2_FOUND)
        target_include_directories(kinnie PRIVATE ${SDL2_INCLUDE_DIRS})
    endif()
    pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
    if(SDL2_TTF_FOUND)
        target_include_directories(kinnie PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
    endif()
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(kinnie PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

install(TARGETS kinnie RUNTIME DESTINATION bin)
