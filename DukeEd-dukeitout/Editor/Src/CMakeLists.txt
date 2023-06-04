project(Editor CXX)

file(GLOB SOURCES *.cpp)
source_group("Source Files" FILES ${SOURCES})

file(GLOB HEADERS ../Inc/*.h *.h)
source_group("Header Files" FILES ${HEADERS})

add_library(Editor SHARED
    ${SOURCES}
    ${HEADERS}
)

set_property(TARGET Editor PROPERTY FOLDER "UnrealEngine")

target_compile_definitions(Editor PRIVATE
    "_UNICODE"
	"UNICODE"
    "EDITOR_API=__declspec(dllexport)"
)

target_link_libraries(Editor PRIVATE
    Engine
)

target_include_directories(Editor PRIVATE
    ./
    ../Inc
    ../../Core/Inc
    ../../Engine/Inc
    ../../Window/Inc
)
