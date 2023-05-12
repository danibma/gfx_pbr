cmake_minimum_required(VERSION 3.22)

project(gfx_pbr)

# Files
file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.geom
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)

file(GLOB_RECURSE HEADER_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

file(GLOB_RECURSE INLINE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl)

file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)


add_executable(gfx_pbr ${SOURCE_FILES})

target_sources(gfx_pbr PRIVATE ${SHADER_FILES})
source_group("Shaders" FILES ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES VS_TOOL_OVERRIDE "None")

target_sources(gfx_pbr PRIVATE ${HEADER_FILES} ${INLINE_FILES} ${SHADER_FILES})
target_include_directories(gfx_pbr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Set up startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT gfx_pbr)

set(GFX_BUILD_EXAMPLES            OFF CACHE BOOL "Build gfx examples")
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "Build loader_example")

# link gfx
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/gfx)
set_target_properties(gfx PROPERTIES FOLDER "external")
target_link_libraries(gfx_pbr PUBLIC gfx)

# Organize third party projects
set_target_properties(uninstall PROPERTIES FOLDER "third_party")
set_target_properties(gfx PROPERTIES FOLDER "third_party")
set_target_properties(tinyobjloader PROPERTIES FOLDER "third_party/gfx_deps")
set_target_properties(ktx PROPERTIES FOLDER "third_party/gfx_deps")
set_target_properties(astcenc-avx2-static PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
set_target_properties(ktx_read PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
set_target_properties(ktx_version PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
set_target_properties(obj_basisu_cbind PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
set_target_properties(objUtil PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")

if(TARGET mkvk)
    set_target_properties(mkvk PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
    set_target_properties(makedfd2vk PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
    set_target_properties(makevk2dfd PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
    set_target_properties(mkvkformatfiles PROPERTIES FOLDER "third_party/gfx_deps/ktx_deps")
endif()

target_compile_features(gfx_pbr PUBLIC cxx_std_20)
target_compile_options(gfx_pbr PRIVATE
    -DUSE_PIX
    /W3 /WX
    -D_HAS_EXCEPTIONS=0
)

set_target_properties(gfx_pbr PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_command(
    TARGET gfx_pbr
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxil.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxil.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxil.dll)

add_custom_command(
    TARGET gfx_pbr
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/dxcompiler.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_DXC_PATH}/bin/x64/dxcompiler.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/dxcompiler.dll)

add_custom_command(
    TARGET gfx_pbr
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Debug/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/Release/WinPixEventRuntime.dll
    COMMAND ${CMAKE_COMMAND} -E copy
            ${GFX_PIX_PATH}/bin/x64/WinPixEventRuntime.dll
            ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/WinPixEventRuntime.dll)
