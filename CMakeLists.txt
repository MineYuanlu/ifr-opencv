cmake_minimum_required(VERSION 3.5)
#设置项目名称
set(PROJECT_NAME IFR_OPENCV)
#建立项目
project(${PROJECT_NAME})
#C++ 17
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O2")
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_FLAGS -pthread)
set(CMAKE_CXX_FLAGS -pthread)

set(CMAKE_SYSROOT "${SYSROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH "${SYSROOT_PATH}" "${CMAKE_PREFIX_PATH}" "${TOOLCHAIN_PATH}")


add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    add_compile_options(-Wall -Werror=return-type -Wno-unused-variable)
    if (CMAKE_BUILD_TYPE STREQUAL Release)
        add_compile_options(-O3 -flto -march=native)
        add_definitions(-DEBUG=0)
    elseif (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
        add_compile_options(-O3 -flto -march=native -g)
        add_definitions(-DEBUG=0)
    elseif (CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
        add_compile_options(-Os -flto -march=native)
        add_definitions(-DEBUG=0)
    else ()
        add_compile_options(-O0 -g)
        add_definitions(-DEBUG=1)
    endif ()
else()
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(DEBUG=1)
endif ()

#寻找CUDA库
find_package(CUDA REQUIRED)
message(STATUS "CUDA version: ${CUDA_VERSION}")
message(STATUS "    include path: ${CUDA_INCLUDE_DIRS}")
message(STATUS "    libraries: ${CUDA_LIBRARIES}")
set(CUDA_NVCC_FLAGS -G;-g;-std=c++${CMAKE_CXX_STANDARD}) # nvcc flags
include_directories(${CUDA_INCLUDE_DIRS})

#寻找OpenCV库
FIND_PACKAGE(OpenCV REQUIRED)
# directory of opencv headers
include_directories(${OpenCV_INCLUDE_DIRS})
# directory of opencv library
link_directories(${OpenCV_LIBRARY_DIRS})

#打印调试信息
MESSAGE(STATUS "Project: ${PROJECT_NAME}")
MESSAGE(STATUS "OpenCV library status:")
MESSAGE(STATUS "    version: ${OpenCV_VERSION}")
MESSAGE(STATUS "    libraries: ${OpenCV_LIBS}")
MESSAGE(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")


include_directories(lib/UltraMultiThread/include)
include_directories(lib/basic)
include_directories(lib)
include_directories(lib/ifr-modules/lib)
include_directories(lib/ifr-modules/modules)


aux_source_directory(${PROJECT_SOURCE_DIR}/lib/basic LIB_BASIC)
aux_source_directory(${PROJECT_SOURCE_DIR}/lib/web LIB_WEB)
aux_source_directory(${PROJECT_SOURCE_DIR}/lib/rapidjson LIB_JSON)
aux_source_directory(${PROJECT_SOURCE_DIR}/lib/predict_needed LIB_PREDICT)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCX)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/tasks SRC_TASK)
aux_source_directory(${PROJECT_SOURCE_DIR}/lib/ifr-modules LIB_MODULES)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/ifr-modules/modules)
add_executable(${PROJECT_NAME} ${SRCX} ${SRC_TASK} ${LIB_BASIC} ${LIB_WEB} ${LIB_JSON} ${LIB_PREDICT})
#添加链接库

target_link_libraries(${PROJECT_NAME} common_modules)

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS} gxiapi.so -lpthread -lm)
else()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS} "C:\\Program Files\\Daheng Imaging\\GalaxySDK\\Samples\\VC SDK\\lib\\x64\\GxIAPI.lib")
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/EHsc")
endif ()