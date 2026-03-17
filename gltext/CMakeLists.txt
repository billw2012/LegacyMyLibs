project(gltext)

add_definitions(-DHAVE_OT=1)

# Only the core harfbuzz sources — excludes optional glib/icu/graphite2/gobject/uniscribe
set(HB_SOURCES
    harfbuzz/hb-blob.cc
    harfbuzz/hb-buffer.cc
    harfbuzz/hb-common.cc
    harfbuzz/hb-fallback-shape.cc
    harfbuzz/hb-font.cc
    harfbuzz/hb-ft.cc
    harfbuzz/hb-ot-layout.cc
    harfbuzz/hb-ot-map.cc
    harfbuzz/hb-ot-shape-complex-arabic.cc
    harfbuzz/hb-ot-shape-complex-indic.cc
    harfbuzz/hb-ot-shape-complex-misc.cc
    harfbuzz/hb-ot-shape-normalize.cc
    harfbuzz/hb-ot-shape.cc
    harfbuzz/hb-ot-tag.cc
    harfbuzz/hb-shape.cc
    harfbuzz/hb-unicode.cc
)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/harfbuzz
)

target_link_libraries(gltext PRIVATE ${FREETYPE_LIBRARIES})
