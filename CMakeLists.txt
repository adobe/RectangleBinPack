cmake_minimum_required(VERSION 3.10)

project(RectangleBinPack VERSION "1.0.0" LANGUAGES CXX)

option(RECTANGLE_BIN_PACK_BUILD_TESTS "Build RectangleBinPack tests" OFF)
option(RECTANGLE_BIN_PACK_BUILD_EXAMPLES "Build RectangleBinPack examples" OFF)

add_library(RectangleBinPack
    GuillotineBinPack.cpp
    MaxRectsBinPack.cpp
    Rect.cpp
    ShelfBinPack.cpp
    ShelfNextFitBinPack.cpp
    SkylineBinPack.cpp
)
add_library(RectangleBinPack::RectangleBinPack ALIAS RectangleBinPack)
target_compile_features(RectangleBinPack PUBLIC cxx_std_11)

# Expose <RectangleBinPack/Foo.h> without moving the original headers.
set(RECTANGLE_BIN_PACK_HEADERS
    GuillotineBinPack.h
    MaxRectsBinPack.h
    Rect.h
    ShelfBinPack.h
    ShelfNextFitBinPack.h
    SkylineBinPack.h
)
foreach(header IN LISTS RECTANGLE_BIN_PACK_HEADERS)
    configure_file("${header}"
        "${CMAKE_CURRENT_BINARY_DIR}/include/RectangleBinPack/${header}" COPYONLY)
endforeach()

target_include_directories(RectangleBinPack PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

if(RECTANGLE_BIN_PACK_BUILD_TESTS)
    enable_testing()

    add_executable(MaxRectsBinPackTest test/MaxRectsBinPackTest.cpp)
    target_link_libraries(MaxRectsBinPackTest PRIVATE RectangleBinPack::RectangleBinPack)
    add_test(NAME MaxRectsBinPackTest COMMAND MaxRectsBinPackTest --smoke)

    add_executable(RectangleBinPackSmokeTest test/SmokeTest.cpp)
    target_link_libraries(RectangleBinPackSmokeTest PRIVATE RectangleBinPack::RectangleBinPack)
    add_test(NAME RectangleBinPackSmokeTest COMMAND RectangleBinPackSmokeTest)
endif()

if(RECTANGLE_BIN_PACK_BUILD_EXAMPLES)
    add_executable(RectangleBinPackExample MaxRectsBinPackTest/BinPack.cpp)
    target_link_libraries(RectangleBinPackExample PRIVATE RectangleBinPack::RectangleBinPack)
endif()
