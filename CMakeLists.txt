cmake_minimum_required(VERSION 3.19)

include(kvasir/cmake/kvasir.cmake)

project("FlipDot Matrix" VERSION 0.1.0)

include(kvasir/cmake/util.cmake)

add_executable(development src/main.cpp)
target_configure_kvasir(development
    OPTIMIZATION_STRATEGY size
    USE_LOG
)

add_subdirectory(aglio)
add_subdirectory(recreated_goon)

recreated_goon_generate(
        type_descriptors
        OUTPUT_DIRECTORY type_descriptors
        FILES src/package.hpp
)

#add_subdirectory(mcufont)

#mcufont_generate(sub_font
#        FONT Hack-Regular.ttf
#        SIZE 8
#        ITERATIONS 20
#        RANGES 0-127)

#mcufont_add_library(development FONTS sub_font)

target_link_libraries(development aglio type_descriptors)

add_executable(release src/main.cpp)
target_link_libraries(release aglio type_descriptors)
target_configure_kvasir(release
    OPTIMIZATION_STRATEGY size
)
#mcufont_add_library(release FONTS sub_font)