cmake_minimum_required (VERSION 3.18)
project(OpenGL_3D_Scene)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)


file(GLOB_RECURSE SHADER_FILES
        "data/*.vert"
        "data/*.frag"
        "data/*.comp"
        "data/*.geom"
        )
if(MSVC)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    else()
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
    endif()
elseif(UNIX)
    set(GLSL_VALIDATOR "glslangValidator")
endif()
foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    get_filename_component(PATH_NAME ${SHADER} DIRECTORY)
    get_filename_component(EXTENSION ${SHADER} EXT)
    file(RELATIVE_PATH PATH_NAME "${CMAKE_CURRENT_SOURCE_DIR}" ${PATH_NAME})
    #MESSAGE("Data PATH: ${PATH_NAME} NAME: ${FILE_NAME}")
    set(SHADER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PATH_NAME}/${FILE_NAME}")
    #MESSAGE("Data OUT PATH: ${DATA_OUTPUT}")
    add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E copy
            ${SHADER}
            ${SHADER_OUTPUT}
            COMMAND ${GLSL_VALIDATOR}  ${SHADER}
            DEPENDS ${SHADER})
    list(APPEND SCRIPT_OUTPUT_FILES ${SHADER_OUTPUT})
endforeach(SHADER)

add_custom_target(shader_target
        DEPENDS ${SCRIPT_OUTPUT_FILES}
)

file(GLOB_RECURSE DATA_FILES
            "data/*.json"
            "data/*.png"
            "data/*.jpg"
            "data/*.bmp"
            "data/*.hdr"
            "data/*.obj"
            "data/*.mtl"
            )
    foreach(DATA ${DATA_FILES})
        get_filename_component(FILE_NAME ${DATA} NAME)
        get_filename_component(PATH_NAME ${DATA} DIRECTORY)
        get_filename_component(EXTENSION ${DATA} EXT)
        file(RELATIVE_PATH PATH_NAME "${CMAKE_CURRENT_SOURCE_DIR}" ${PATH_NAME})
        #MESSAGE("Data PATH: ${PATH_NAME} NAME: ${FILE_NAME}")
        set(DATA_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${PATH_NAME}/${FILE_NAME}")
        #MESSAGE("Data OUT PATH: ${DATA_OUTPUT}")
        add_custom_command(
                OUTPUT ${DATA_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E copy
                ${DATA}
                ${DATA_OUTPUT}
                DEPENDS ${DATA})
        list(APPEND Data_OUTPUT_FILES ${DATA_OUTPUT})
    endforeach(DATA)


    add_custom_target(
            data_target
            DEPENDS ${Data_OUTPUT_FILES}
    )

file(GLOB_RECURSE COMMON_FILES common/include/*.h common/src/*.cpp)
add_library(common ${COMMON_FILES})
set_target_properties(common PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(common PUBLIC common/include/)

file(GLOB_RECURSE GRAPHICS_FILES core/include/*.h core/src/*.cpp)
add_library(core ${GRAPHICS_FILES})
set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(core PUBLIC core/include/)
target_link_libraries(core PUBLIC GLEW::GLEW glm::glm SDL2::SDL2 SDL2::SDL2main imgui::imgui fmt::fmt assimp::assimp)
target_link_libraries(core PRIVATE common)

file(GLOB_RECURSE ENGINE_FILES main/include/*.h main/src/*.cpp)
add_library(scenes STATIC ${ENGINE_FILES} ${SHADER_FILES} ${SCENE_FILES})
target_include_directories(scenes PRIVATE ${Stb_INCLUDE_DIR})
target_include_directories(scenes PUBLIC main/include/)
target_link_libraries(scenes PUBLIC common core)
set_target_properties(scenes PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(scenes PROPERTIES UNITY_BUILD ON)
add_dependencies(scenes shader_target data_target)

if(MSVC)
    target_compile_definitions(scenes PUBLIC "_USE_MATH_DEFINES" WIN32_LEAN_AND_MEAN)
    target_compile_options(scenes PUBLIC /arch:AVX2 /Oi /GL /fp:fast)
    target_link_options(scenes PUBLIC /LTCG)
else()

endif()

add_executable(main main/main.cpp)
target_link_libraries(main PRIVATE scenes)