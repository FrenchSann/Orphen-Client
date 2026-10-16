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

# Small Java bridge: one method-entry callback, transformed with an isolated ASM.
find_package(Java 11 REQUIRED COMPONENTS Development)
set(BRIDGE_CLASSES ${CMAKE_BINARY_DIR}/bridge-classes)
set(BRIDGE_JAR ${CMAKE_BINARY_DIR}/bin/orphen-bridge.jar)
file(MAKE_DIRECTORY ${BRIDGE_CLASSES} ${CMAKE_BINARY_DIR}/bin)
add_custom_command(OUTPUT ${BRIDGE_JAR}
    COMMAND ${Java_JAR_EXECUTABLE} xf ${EXT}/asm/asm-9.8.jar
    COMMAND ${CMAKE_COMMAND} -E rm -f ${BRIDGE_CLASSES}/module-info.class
    COMMAND ${Java_JAVAC_EXECUTABLE} --release 8 -Xlint:-options
        -cp ${EXT}/asm/asm-9.8.jar -d ${BRIDGE_CLASSES}
        ${CMAKE_SOURCE_DIR}/bridge/orphen/bridge/TickBridge.java
        ${CMAKE_SOURCE_DIR}/bridge/orphen/bridge/Transformer.java
    COMMAND ${CMAKE_COMMAND} -E copy ${EXT}/asm/LICENSE.html ${BRIDGE_CLASSES}/META-INF/ASM-LICENSE.html
    COMMAND ${Java_JAR_EXECUTABLE} cf ${BRIDGE_JAR} -C ${BRIDGE_CLASSES} .
    WORKING_DIRECTORY ${BRIDGE_CLASSES}
    DEPENDS bridge/orphen/bridge/TickBridge.java bridge/orphen/bridge/Transformer.java
        ${EXT}/asm/asm-9.8.jar ${EXT}/asm/LICENSE.html
    VERBATIM)
add_custom_target(orphen_bridge ALL DEPENDS ${BRIDGE_JAR})

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
add_dependencies(orphen orphen_bridge)
target_include_directories(orphen PRIVATE ${CMAKE_SOURCE_DIR}/src ${EXT}/jni)
target_link_libraries(orphen PRIVATE minhook imgui opengl32)

# --- Injecteur ---
add_executable(injector ${CMAKE_SOURCE_DIR}/injector/main.cpp)
set_target_properties(injector PROPERTIES OUTPUT_NAME OrphenClient)
set_target_properties(orphen injector PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

option(ORPHEN_BUILD_TESTS "Build the isolated JVM class-loader test" OFF)
if(ORPHEN_BUILD_TESTS)
    add_executable(classloader_test tests/classloader_test.cpp)
    target_include_directories(classloader_test PRIVATE ${CMAKE_SOURCE_DIR}/src ${EXT}/jni)
    add_executable(tick_hook_test tests/tick_hook_test.cpp src/core/tick_hook.cpp)
    target_include_directories(tick_hook_test PRIVATE ${CMAKE_SOURCE_DIR}/src ${EXT}/jni)
    add_dependencies(tick_hook_test orphen_bridge)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/test-fixture/TickFixture.class
        COMMAND ${Java_JAVAC_EXECUTABLE} --release 8 -Xlint:-options
        -d ${CMAKE_BINARY_DIR}/test-fixture ${CMAKE_SOURCE_DIR}/tests/TickFixture.java
        DEPENDS tests/TickFixture.java
        VERBATIM)
    add_custom_target(tick_fixture DEPENDS ${CMAKE_BINARY_DIR}/test-fixture/TickFixture.class)
    add_dependencies(tick_hook_test tick_fixture)
endif()
