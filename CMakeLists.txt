cmake_minimum_required(VERSION 3.22)

project(raylib_base LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)


# -- deps --
set(BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)  # raylib: skip its example programs
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # raylib: static, no runtime .dylib juggling
set(GLM_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/raylib EXCLUDE_FROM_ALL)
add_subdirectory(external/glm    EXCLUDE_FROM_ALL)


# -- source files --
file( GLOB_RECURSE APP_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)


# -- executable --
add_executable(raylib_base ${APP_SRC})

# -- warnings --
target_compile_options(raylib_base PRIVATE
    -Wall
    -Wswitch
    -Werror
    -Wimplicit-fallthrough
    -Wno-unused
    -Wunused-result
#    -Wfatal-errors
    -fdiagnostics-color=always
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=. # gives stack traces a relative path to cwd
)


# -- includes --
target_include_directories(raylib_base PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties( raylib_base PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)


# ===========
# Precompiled header (pch.hpp)
# ===========
target_precompile_headers(raylib_base PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.hpp"
)

target_link_libraries(raylib_base PRIVATE
    raylib
    glm::glm
)

# --- Optimization and sanitizers

OPTION(MC_ENABLE_ASAN  "ENABLE ADDRESSSANITIZER" OFF)
OPTION(MC_ENABLE_LSAN  "ENABLE ADDRESSSANITIZER LEAK DETECTION" OFF)
OPTION(MC_ENABLE_TSAN  "ENABLE THREADSANITIZER" OFF)
OPTION(MC_ENABLE_UBSAN "ENABLE UBSAN" OFF)
OPTION(MC_FAST_MATH    "ENABLE -FFAST-MATH" OFF)
OPTION(MC_O1 "ENABLE -O1" OFF)
OPTION(MC_O2 "ENABLE -O2" OFF)
OPTION(MC_O3 "ENABLE -O3" OFF)

if(MC_O1)
    target_compile_options(raylib_base PRIVATE -O1)
endif()

if(MC_O2)
    target_compile_options(raylib_base PRIVATE -O2)
endif()

if(MC_O3)
    target_compile_options(raylib_base PRIVATE -O3)
endif()

if(MC_FAST_MATH)
    target_compile_options(raylib_base PRIVATE -ffast-math)
endif()

if(MC_ENABLE_ASAN OR MC_ENABLE_LSAN)
    target_compile_options(raylib_base PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(raylib_base PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
endif()

if(MC_ENABLE_TSAN)
    target_compile_options(raylib_base PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
    target_link_options(raylib_base PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
endif()

if(MC_ENABLE_UBSAN)
    target_compile_options(raylib_base PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
    target_link_options(raylib_base PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
endif()

add_custom_target(run
    COMMAND sh -c "./bin/raylib_base"
    DEPENDS raylib_base
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(debug
    COMMAND lldb -o run -- "$<TARGET_FILE:raylib_base>"
    DEPENDS raylib_base
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    USES_TERMINAL
)
