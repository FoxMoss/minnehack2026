add_executable(fly
 main.cpp
 raygui.cpp
)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

if(EMSCRIPTEN)
  add_definitions(-DFLIGHT_SIM_STATIC)
  set(CMAKE_BUILD_TYPE Web)
endif()

set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()


target_link_libraries(fly raylib nlohmann_json::nlohmann_json)


set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)


if (${PLATFORM} STREQUAL "Web")

    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html") # Tell Emscripten to build an example.html file.
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY -s GL_ENABLE_GET_PROC_ADDRESS=1 --preload-file ../../resources@/  -sSTACK_SIZE=131072 -sINITIAL_MEMORY=171048960 -sEXPORTED_RUNTIME_METHODS=HEAPF32,cwrap -sEXPORTED_FUNCTIONS=_send_inputs,_get_inputs,_main --shell-file=../../../phone-emscripten-shell/index.html")
endif()

add_custom_command(TARGET fly POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMENT "Add clangd to the base directory")

add_custom_command(TARGET fly POST_BUILD
  COMMAND mkdir -p ${CMAKE_BINARY_DIR}/built -p && cp ${CMAKE_SOURCE_DIR}/../phone-emscripten-shell/* -r ${CMAKE_BINARY_DIR}/built/ && cp
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/*
      ${CMAKE_BINARY_DIR}/built && mv ${CMAKE_BINARY_DIR}/built/${PROJECT_NAME}.html ${CMAKE_BINARY_DIR}/built/index.html
        COMMENT "Make build dir")


install(TARGETS fly DESTINATION bin)
