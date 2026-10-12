cmake_minimum_required(VERSION 3.10)
project(RectangleBinPackConsumer LANGUAGES CXX)

enable_testing()
add_subdirectory(../.. rectanglebinpack)

foreach(target IN ITEMS MaxRectsBinPackTest RectangleBinPackSmokeTest RectangleBinPackExample)
    if(TARGET ${target})
        message(FATAL_ERROR "Library-only builds must not create ${target}")
    endif()
endforeach()

add_executable(consumer ../SmokeTest.cpp)
target_link_libraries(consumer PRIVATE RectangleBinPack::RectangleBinPack)
add_test(NAME consumer COMMAND consumer)
