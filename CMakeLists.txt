cmake_minimum_required(VERSION 3.5)
#设置项目名称
set(PROJECT_NAME IFR_OPENCV)
#建立项目
project(${PROJECT_NAME})
#C++ 14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O2")
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_C_FLAGS -pthread)
set(CMAKE_CXX_FLAGS -pthread)

set(CMAKE_SYSROOT "${SYSROOT_PATH}")
set(CMAKE_FIND_ROOT_PATH "${SYSROOT_PATH}" "${CMAKE_PREFIX_PATH}" "${TOOLCHAIN_PATH}")


add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_compile_options(-Wall -Werror=return-type -Wno-unused-variable)
if (CMAKE_BUILD_TYPE STREQUAL Release)
    add_compile_options(-O3 -flto -march=native)
elseif (CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    add_compile_options(-O3 -flto -march=native -g)
elseif (CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
    add_compile_options(-Os -flto -march=native)
else ()
    add_compile_options(-O0 -g)
endif ()

#寻找CUDA库
find_package(CUDA REQUIRED)
message(STATUS "CUDA version: ${CUDA_VERSION}")
message(STATUS "    include path: ${CUDA_INCLUDE_DIRS}")
message(STATUS "    libraries: ${CUDA_LIBRARIES}")
set(CUDA_NVCC_FLAGS -G;-g;-std=c++14) # nvcc flags
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


aux_source_directory(${PROJECT_SOURCE_DIR}/lib/basic LIB_BASIC)
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCX)
add_executable(${PROJECT_NAME} src/main.cpp src/FinderEM.cpp src/DataWaiter.h src/AimEM.cpp src/AimEM.h src/Camera.cpp ${SRCX} ${LIB_BASIC} src/API.cpp src/API.h)
#添加链接库
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS} gxiapi.so -lpthread -lm)


# opencv libraries
#target_link_libraries(${PROJECT_NAME} ${OpenCV _LIBS} libgxiapi.so)

#获取代码，在项目中，将所有代码都放在src文件夹中
#AUX_SOURCE_DIRECTORY(src DIR_SRCS)
#MESSAGE(STATUS "Src file: ${DIR_SRCS}")
#编译可执行程序
#ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS})


##add_executable(test main.cpp)

