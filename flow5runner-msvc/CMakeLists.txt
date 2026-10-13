cmake_minimum_required(VERSION 3.20)

project(Flow5Runner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLOW5_SOURCE_DIR "" CACHE PATH "Root directory of the flow5 source tree")
set(FLOW5_LIB_DIR "" CACHE PATH "Directory containing flow5-lib.lib")
set(FLOW5_IO_LIB_DIR "" CACHE PATH "Directory containing flow5-io-lib.lib")
set(XFOIL_LIB_DIR "" CACHE PATH "Directory containing XFoil1.lib")
set(GMSH_INCLUDE_DIR "" CACHE PATH "Directory containing gmsh.h")
set(GMSH_LIB_DIR "" CACHE PATH "Directory containing gmsh.dll.lib and gmsh.dll")
set(QT_ROOT "" CACHE PATH "Qt MSVC installation root")
set(OCCT_ROOT "" CACHE PATH "OpenCascade installation root")
set(MKL_ROOT "" CACHE PATH "Intel oneAPI MKL installation root")
set(ONEAPI_ROOT "" CACHE PATH "Intel oneAPI installation root")

foreach(required FLOW5_SOURCE_DIR FLOW5_LIB_DIR FLOW5_IO_LIB_DIR XFOIL_LIB_DIR GMSH_INCLUDE_DIR GMSH_LIB_DIR QT_ROOT OCCT_ROOT MKL_ROOT ONEAPI_ROOT)
    if("${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} must be supplied")
    endif()
endforeach()

add_executable(Flow5Runner Flow5Runner.cpp)

target_include_directories(Flow5Runner PRIVATE
    "${FLOW5_SOURCE_DIR}/XFoil-lib"
    "${FLOW5_SOURCE_DIR}/flow5-lib"
    "${FLOW5_SOURCE_DIR}/flow5-lib/api"
    "${FLOW5_SOURCE_DIR}/flow5-io-lib/api"
    "${GMSH_INCLUDE_DIR}"
    "${QT_ROOT}/include"
    "${QT_ROOT}/include/QtCore"
    "${OCCT_ROOT}/inc"
    "${MKL_ROOT}/include"
)

target_compile_definitions(Flow5Runner PRIVATE WIN_OS INTEL_MKL)
target_compile_options(Flow5Runner PRIVATE /EHsc /utf-8 /Zc:__cplusplus /permissive-)

target_link_directories(Flow5Runner PRIVATE
    "${FLOW5_LIB_DIR}"
    "${FLOW5_IO_LIB_DIR}"
    "${XFOIL_LIB_DIR}"
    "${GMSH_LIB_DIR}"
    "${QT_ROOT}/lib"
    "${OCCT_ROOT}/win64/vc14/libd"
    "${MKL_ROOT}/lib"
    "${ONEAPI_ROOT}/compiler/latest/lib"
)

target_link_libraries(Flow5Runner PRIVATE
    flow5-lib
    flow5-io-lib
    XFoil1
    gmsh.dll.lib
    TKernel
    TKMath
    TKG2d
    TKG3d
    TKGeomBase
    TKBRep
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    TKBO
    TKShHealing
    TKBool
    TKHLR
    TKMesh
    TKFillet
    TKOffset
    TKFeat
    TKXSBase
    TKDESTEP
    TKDEIGES
    TKDEOBJ
    TKDESTL
    Qt6Core
    mkl_intel_lp64_dll
    mkl_core_dll
    mkl_intel_thread_dll
    libiomp5md
)
