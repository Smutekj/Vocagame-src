cmake_minimum_required(VERSION 3.15)

project(projectx)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

include(cmake/External.cmake)
include(cmake/CompilerFlags.cmake)

file(GLOB SRC_CLIENT
     "src/*.cpp"
     "src/Utils/*.cpp"
     "src/Menu/*.cpp"
     "src/Entities/*.cpp"
     "src/Systems/*.cpp"
     "src/Editor/*.cpp"
     "src/Client/*.cpp"
     "src/Games/*.cpp"
     "src/PathFinding/*.cpp"
     "src/Shadows/*.cpp"
)

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(${CMAKE_SOURCE_DIR}/Renderer)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Android")
     set(PROJECT_NAME main)
     add_library(main SHARED ${SRC} )
else()
     set(PROJECT_NAME ${CMAKE_PROJECT_NAME})
     add_executable(${PROJECT_NAME}_Client ${SRC_CLIENT} )
endif()

target_compile_options(${PROJECT_NAME}_Client PRIVATE "-Wnon-virtual-dtor")

target_include_directories(${PROJECT_NAME}_Client PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Utils>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Systems>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Menu>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Editor>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Client>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Games>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Entities>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/PathFinding>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Shadows>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Renderer/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external/boost>
)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
target_link_libraries(${PROJECT_NAME}_Client PRIVATE renderer CDT idbfs.js nlohmann_json::nlohmann_json)# piper onnxruntime)
else()
target_link_libraries(${PROJECT_NAME}_Client PRIVATE renderer CDT nlohmann_json::nlohmann_json)# piper onnxruntime)
endif()

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
     target_compile_definitions(${PROJECT_NAME}_Client PRIVATE RESOURCES_DIR="/Resources/") ### add correct resources path here
elseif( ${CMAKE_SYSTEM_NAME} MATCHES "Android" )
     target_compile_definitions(${PROJECT_NAME} PRIVATE RESOURCES_DIR="")
else()
     target_compile_definitions(${PROJECT_NAME}_Client PRIVATE RESOURCES_DIR="${CMAKE_SOURCE_DIR}/Resources/" $<$<CONFIG:Debug>:DEBUG>) ### add correct resources path here
endif()

set_target_compiler_flags(${PROJECT_NAME}_Client)

if( ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")

     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --profiling  -sUSE_SDL=2")
     set(CMAKE_EXECUTABLE_SUFFIX .js)
     target_link_options(${PROJECT_NAME}_Client PRIVATE
     "-sUSE_SDL=2"
     "-sUSE_SDL_MIXER=2"
     "-sFULL_ES3=1"
     "-sWASM=1"
     "-sUSE_WEBGL2=1"
     "-sEXPORTED_RUNTIME_METHODS=['ccall','requestFullscreen', 'callMain']"
     "-sEXPORTED_FUNCTIONS=['_main', '_receiveMessage', '_emscripten_pause_main_loop','_emscripten_resume_main_loop','_restartGame','_startGame', '_setCanvasSize','_disableInput','_toggleFullscreen','_zoomCamera','_assetsLoaded']"
     "-sMIN_WEBGL_VERSION=2"
     "-sMAX_WEBGL_VERSION=2"
     "-sASSERTIONS=1"
     "-sINVOKE_RUN=0"
     "-sUSE_PTHREADS=0"
     "-sPTHREAD_POOL_SIZE=0"
     "-sPROXY_TO_PTHREAD=0"
     "-sPROXY_TO_WORKER=0"
     "-sFETCH=1"
     # "-sPROXY_POSIX_SOCKETS=0"
     "-sOFFSCREEN_FRAMEBUFFER=1"
     "-sMODULARIZE=0"
     "-o=projectx.js"
     "-sALLOW_MEMORY_GROWTH=1"
     "-sASYNCIFY=0"
     # "-sASYNCIFY_STACK_SIZE=65536"
     "-s EXIT_RUNTIME=1" 
     "-sEXCEPTION_CATCHING_ALLOWED=yes" 
     )
     target_link_options(${PROJECT_NAME}_Client PRIVATE 
      --embed-file=${CMAKE_CURRENT_SOURCE_DIR}/external/lygia@../external/lygia
     --embed-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shaders@../Resources/Shaders
     --embed-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources/Resources.json@../Resources/Resources.json
     --embed-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources/TextResources.json@../Resources/TextResources.json
     --embed-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources/Levels@../Resources/Levels
     --embed-file=${CMAKE_CURRENT_SOURCE_DIR}/Resources/execDb@../Resources/execDb
     --post-js=${CMAKE_CURRENT_SOURCE_DIR}/patchBrowserUpdateScreen.js
     )
endif()

