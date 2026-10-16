cmake_minimum_required(VERSION 3.20)
project(orphen_client CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Build en x64 uniquement (la JVM de Minecraft est 64 bits) : cmake -A x64")
endif()

add_compile_options(/utf-8 /W3 /MP)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

set(EXT ${CMAKE_SOURCE_DIR}/external)

# --- MinHook ---
file(GLOB MINHOOK_SRC ${EXT}/minhook/src/*.c ${EXT}/minhook/src/hde/*.c)
add_library(minhook STATIC ${MINHOOK_SRC})
target_include_directories(minhook PUBLIC ${EXT}/minhook/include)

# --- Dear ImGui (Win32 + OpenGL2 : Minecraft 1.8.9 = LWJGL2, contexte GL legacy) ---
add_library(imgui STATIC
    ${EXT}/imgui/imgui.cpp
    ${EXT}/imgui/imgui_draw.cpp
    ${EXT}/imgui/imgui_tables.cpp
    ${EXT}/imgui/imgui_widgets.cpp
    ${EXT}/imgui/backends/imgui_impl_win32.cpp
    ${EXT}/imgui/backends/imgui_impl_opengl2.cpp)
target_include_directories(imgui PUBLIC ${EXT}/imgui ${EXT}/imgui/backends)

# --- Orphen Client (DLL) ---
file(GLOB_RECURSE UTIL_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_library(orphen SHARED ${UTIL_SRC})
target_include_directories(orphen PRIVATE ${CMAKE_SOURCE_DIR}/src ${EXT}/jni)
target_link_libraries(orphen PRIVATE minhook imgui opengl32)

# --- Injecteur ---
add_executable(injector ${CMAKE_SOURCE_DIR}/injector/main.cpp)
set_target_properties(injector PROPERTIES OUTPUT_NAME OrphenClient)
set_target_properties(orphen injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
