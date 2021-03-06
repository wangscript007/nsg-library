get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
PROJECT(${PROJECT_NAME})

file(GLOB src "*.cpp")
file(GLOB hdr "*.h")

set (ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

file(GLOB shaders ${ASSETS_DIR}/shaders/*.glsl)
source_group("shaders" FILES ${shaders})

include_directories(${ASSETS_DIR})

macro(generate_shader_header_file input_file input_template_file output_file)
    file(STRINGS ${input_file} shader_file_lines)
    unset(shader_string_file)
    set(shader_string_file \\\n)
    foreach(shader_file_line ${shader_file_lines})
        set(shader_string_file ${shader_string_file}"${shader_file_line}\\n"\\\n)
    endforeach()
    configure_file(${input_template_file} ${output_file})
endmacro()

generate_shader_header_file(${ASSETS_DIR}/shaders/Common.glsl ${ASSETS_DIR}/shaders/Common.in ${ASSETS_DIR}/autogenerated/Common_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/Samplers.glsl ${ASSETS_DIR}/shaders/Samplers.in ${ASSETS_DIR}/autogenerated/Samplers_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/Transforms.glsl ${ASSETS_DIR}/shaders/Transforms.in ${ASSETS_DIR}/autogenerated/Transforms_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/Ambient.glsl ${ASSETS_DIR}/shaders/Ambient.in ${ASSETS_DIR}/autogenerated/Ambient_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/Shadows.glsl ${ASSETS_DIR}/shaders/Shadows.in ${ASSETS_DIR}/autogenerated/Shadows_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/Lighting.glsl ${ASSETS_DIR}/shaders/Lighting.in ${ASSETS_DIR}/autogenerated/Lighting_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/PostProcess.glsl ${ASSETS_DIR}/shaders/PostProcess.in ${ASSETS_DIR}/autogenerated/PostProcess_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/VS.glsl ${ASSETS_DIR}/shaders/VS.in ${ASSETS_DIR}/autogenerated/VS_glsl.inl)
generate_shader_header_file(${ASSETS_DIR}/shaders/FS.glsl ${ASSETS_DIR}/shaders/FS.in ${ASSETS_DIR}/autogenerated/FS_glsl.inl)

subdirlist(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})
FOREACH(subdir ${SUBDIRS})
	file(GLOB tmp_src "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/*.mm" "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/*.c")
	file(GLOB tmp_hdr "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/*.h")
	set(src ${src} ${tmp_src})
	set(hdr ${hdr} ${tmp_hdr})
	source_group(${subdir} FILES ${tmp_hdr} ${tmp_src} ${assets})
ENDFOREACH()

if(ANDROID)
	# See http://developer.android.com/reference/android/app/NativeActivity.html
	list(APPEND hdr $ENV{ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.h)
	list(APPEND src $ENV{ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
	include_directories($ENV{ANDROID_NDK}/sources/android/native_app_glue)
endif()


setup_library()
target_link_libraries(${PROJECT_NAME} ${LIBRARIES_2_LINK})
if(WIN32)
  target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()






