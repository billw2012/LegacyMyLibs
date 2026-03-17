project(gltext)

add_definitions(-DHAVE_OT=1)

file(GLOB HB_SOURCES harfbuzz/*.cc harfbuzz/*.h)

add_library(gltext
    gltext.cpp
    gltext.h
    ${HB_SOURCES}
)

find_package(Freetype REQUIRED)

target_include_directories(gltext
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    PRIVATE
        ${FREETYPE_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(gltext PRIVATE ${FREETYPE_LIBRARIES})
