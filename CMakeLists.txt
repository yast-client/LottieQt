cmake_minimum_required(VERSION 3.5)
project(LottieQt CXX)

find_package(Qt5 COMPONENTS Core Quick REQUIRED)

include(FindPkgConfig)
pkg_search_module(ZLIB zlib REQUIRED)


# tlottie

set(TLOTTIE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tlottie)
set(TLOTTIE_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/tlottie_target)
set(TLOTTIE_LIB ${TLOTTIE_BUILD_DIR}/${TLOTTIE_BUILD_SUBDIR}/release-nostd/libtlottie.a)

add_custom_command(
    OUTPUT ${TLOTTIE_LIB}
    COMMAND ${CMAKE_COMMAND} -E env
        "${TLOTTIE_RUST_ENV}"
        "RUSTFLAGS=-C relocation-model=pic ${TLOTTIE_RUSTFLAGS}"
        "CARGO_TARGET_DIR=${TLOTTIE_BUILD_DIR}"
        cargo rustc --profile release-nostd --features c-api --lib --crate-type staticlib
    WORKING_DIRECTORY ${TLOTTIE_DIR}
)

add_custom_target(tlottie_build DEPENDS ${TLOTTIE_LIB})

add_library(tlottie STATIC IMPORTED)
set_target_properties(tlottie PROPERTIES
    IMPORTED_LOCATION "${TLOTTIE_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${TLOTTIE_DIR}/include"
)
add_dependencies(tlottie tlottie_build)


add_library(LottieQt STATIC)

find_package(Threads)

target_include_directories(LottieQt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(LottieQt
    Qt5::Core
    Qt5::Quick
    ${ZLIB_LIBRARIES}
    tlottie
    ${CMAKE_THREAD_LIBS_INIT} # why is this needed?
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
