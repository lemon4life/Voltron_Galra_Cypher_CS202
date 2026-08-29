if(NOT DEFINED PROJECT_ROOT OR NOT IS_DIRECTORY "${PROJECT_ROOT}")
    message(FATAL_ERROR "PROJECT_ROOT must point to the repository root")
endif()

# Keep CI synchronized with directly referenced runtime assets without
# maintaining a second handwritten manifest.
file(GLOB_RECURSE PROJECT_SOURCES
    "${PROJECT_ROOT}/src/*.cpp"
    "${PROJECT_ROOT}/include/*.h"
)

set(REQUIRED_ASSETS)
foreach(SOURCE_FILE IN LISTS PROJECT_SOURCES)
    file(READ "${SOURCE_FILE}" SOURCE_CONTENT)
    string(REGEX MATCHALL
        "assets/[A-Za-z0-9_./ -]+\\.(png|ttf|csv)"
        SOURCE_ASSETS
        "${SOURCE_CONTENT}"
    )
    list(APPEND REQUIRED_ASSETS ${SOURCE_ASSETS})
endforeach()

# These paths are assembled at runtime and cannot be discovered from a single
# string literal above.
foreach(INDEX RANGE 1 9)
    list(APPEND REQUIRED_ASSETS "assets/img/Background/bg_${INDEX}.png")
endforeach()
list(APPEND REQUIRED_ASSETS
    "assets/map/hub_Tile Layer 2.csv"
    "assets/map/hub_Tile Game Objects.csv"
)
list(REMOVE_DUPLICATES REQUIRED_ASSETS)
list(SORT REQUIRED_ASSETS)

foreach(ASSET IN LISTS REQUIRED_ASSETS)
    if(NOT EXISTS "${PROJECT_ROOT}/${ASSET}")
        message(FATAL_ERROR "Missing required runtime asset: ${ASSET}")
    endif()
endforeach()
