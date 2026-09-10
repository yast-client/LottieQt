cmake_minimum_required(VERSION 3.5)
project(LottieQt CXX)

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

include(FindPkgConfig)
pkg_search_module(ZLIB zlib REQUIRED)



# rlottie
# rlottie/CMakeLists.txt is too hard to adapt
add_library(rlottie STATIC)

set(RLOTTIE_CONFIG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/config.h)
# we could do file(TOUCH filename), but that requires CMake 3.12
if (NOT EXISTS ${RLOTTIE_CONFIG_PATH})
    file(WRITE ${RLOTTIE_CONFIG_PATH} "")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads)

set_target_properties(rlottie PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(rlottie PUBLIC ${CMAKE_THREAD_LIBS_INIT})

add_definitions(-DLOTTIE_THREAD_SUPPORT)
target_include_directories(rlottie PUBLIC
    rlottie/inc
    rlottie/src/vector
    rlottie/src/vector/freetype
)
target_sources(rlottie
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieanimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieitem_capi.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottiekeypath.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottiemodel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/lottieparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/lottie/zip/zip.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/freetype/v_ft_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/freetype/v_ft_raster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/freetype/v_ft_stroker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/stb/stb_image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/varenaalloc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vbezier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vbitmap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vbrush.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdasher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper_neon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vdrawhelper_sse2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vmatrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vimageloader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vinterpolator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vpainter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vpath.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vpathmesure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vraster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/vrle.cpp
)

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#ifdef __ARM_NEON__
int main() { return 0; }
#else
#error \"NEON not available\"
#endif
" HAVE_NEON)
if (HAVE_NEON)
    message("Using NEON render functions")
    SET(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")
    target_sources(rlottie PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/rlottie/src/vector/pixman/pixman-arm-neon-asm.S
    )
else()
    message("Using default render functions")
endif()



add_library(LottieQt STATIC)

target_include_directories(LottieQt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(LottieQt
    Qt5::Core
    Qt5::Quick
    ${ZLIB_LIBRARIES}
    rlottie
)

target_compile_definitions(LottieQt PUBLIC QT_STATICPLUGIN)

target_sources(LottieQt
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lottieitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tgsplugin.cpp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lottieitem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tgsplugin.h
)

# FIXME?
set_target_properties(LottieQt PROPERTIES AUTOMOC OFF)
set_target_properties(rlottie PROPERTIES AUTOMOC OFF)
