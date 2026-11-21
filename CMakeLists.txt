cmake_minimum_required(VERSION 3.20)

# Project Name is configurable (ulw-ai-agent by default)
set(ULWAI_PROJECT_NAME "ulw-ai-agent" CACHE STRING "Configurable project and binary name")

# Single source of truth for versioning: VERSION file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" ULWAI_VERSION_RAW)
    string(STRIP "${ULWAI_VERSION_RAW}" ULWAI_VERSION)
else()
    set(ULWAI_VERSION "2.0.0")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

project(${ULWAI_PROJECT_NAME}
    VERSION ${ULWAI_VERSION}
    DESCRIPTION "Ultra-Lightweight C++ AI Agent (Terminal & Desktop UI under 100MB RAM)"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for clang-tidy / IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_GUI "Build desktop graphical interface (requires Qt6)" ON)
option(BUILD_TESTS "Build test suite" ON)

# Strict warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ============================================================================
# Dependencies
# ============================================================================

# 1. nlohmann_json
# Check local vendor directory first
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/nlohmann/json.hpp")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/deps")
    message(STATUS "Using vendored nlohmann/json.hpp from deps/nlohmann")
else()
    find_package(nlohmann_json REQUIRED)
endif()

# 2. CURL
find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "Found system CURL: ${CURL_LIBRARIES}")
    set(CURL_TARGET CURL::libcurl)
else()
    find_library(CURL_LIB NAMES curl libcurl libcurl.so.4 libcurl.so PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
    if(CURL_LIB)
        message(STATUS "Found CURL library: ${CURL_LIB} (using vendored headers in deps/curl/include)")
        add_library(vendored_curl UNKNOWN IMPORTED)
        set_target_properties(vendored_curl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/deps/curl/include"
        )
        set(CURL_TARGET vendored_curl)
    else()
        message(FATAL_ERROR "Could not find CURL library. Please install libcurl.")
    endif()
endif()

# 3. Qt (Qt6 preferred, Qt5 fallback for legacy systems)
set(GUI_AVAILABLE FALSE)
set(QT_LIBRARIES "")
if(BUILD_GUI)
    find_package(Qt6 COMPONENTS Widgets Core Gui QUIET)
    if(Qt6_FOUND)
        set(GUI_AVAILABLE TRUE)
        set(QT_LIBRARIES Qt6::Widgets Qt6::Core Qt6::Gui)
        message(STATUS "Qt6 found (${Qt6_VERSION}). Desktop GUI will be built.")
    else()
        find_package(Qt5 5.6 COMPONENTS Widgets Core Gui QUIET)
        if(Qt5_FOUND)
            set(GUI_AVAILABLE TRUE)
            set(QT_LIBRARIES Qt5::Widgets Qt5::Core Qt5::Gui)
            message(STATUS "Qt5 found (${Qt5_VERSION}). Desktop GUI will be built.")
        else()
            message(STATUS "Neither Qt6 nor Qt5 found. Building terminal/CLI version only.")
        endif()
    endif()
endif()

# ============================================================================
# Sources
# ============================================================================

set(CORE_SOURCES
    src/Config.cpp
    src/Platform.cpp
    src/Utils.cpp
    src/Logger.cpp
    src/DuckDuckGoClient.cpp
    src/OpenAIClient.cpp
    src/PollinationsClient.cpp
    src/OllamaClient.cpp
    src/OpenRouterClient.cpp
    src/HuggingfaceClient.cpp
    src/ConversationManager.cpp
    src/cli/CliRunner.cpp
    deps/md4c/md4c.c
)

set(CORE_HEADERS
    include/ulwai/Types.hpp
    include/ulwai/Config.hpp
    include/ulwai/Platform.hpp
    include/ulwai/Utils.hpp
    include/ulwai/Logger.hpp
    include/ulwai/IApiClient.hpp
    include/ulwai/DuckDuckGoClient.hpp
    include/ulwai/OpenAIClient.hpp
    include/ulwai/PollinationsClient.hpp
    include/ulwai/OllamaClient.hpp
    include/ulwai/OpenRouterClient.hpp
    include/ulwai/GrokClient.hpp
    include/ulwai/HuggingfaceClient.hpp
    include/ulwai/ConversationManager.hpp
    include/ulwai/cli/CliRunner.hpp
    include/ulwai/Version.hpp
    include/ulwai/CurlRaii.hpp
    deps/md4c/md4c.h
)

set(GUI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/ChatBubbleView.cpp
    src/ui/SessionSidebar.cpp
    src/ui/SettingsDialog.cpp
)

set(GUI_HEADERS
    include/ulwai/ui/MainWindow.hpp
    include/ulwai/ui/ChatBubbleView.hpp
    include/ulwai/ui/SessionSidebar.hpp
    include/ulwai/ui/SettingsDialog.hpp
    include/ulwai/ui/UiStyles.hpp
)

# Silence compiler warnings in vendored third-party C parser
if(MSVC)
    set_source_files_properties(deps/md4c/md4c.c PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties(deps/md4c/md4c.c PROPERTIES COMPILE_OPTIONS "-w")
endif()

# Core library
add_library(ulwai_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(ulwai_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps
)
target_link_libraries(ulwai_core
    PUBLIC
        ${CURL_TARGET}
        nlohmann_json
        ${CMAKE_DL_LIBS}
)
if(WIN32)
    target_link_libraries(ulwai_core PUBLIC user32 shell32 psapi)
endif()
if(APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
    find_library(SECURITY Security)
    if(CORE_FOUNDATION)
        target_link_libraries(ulwai_core PUBLIC ${CORE_FOUNDATION})
    endif()
    if(SECURITY)
        target_link_libraries(ulwai_core PUBLIC ${SECURITY})
    endif()
endif()
target_compile_definitions(ulwai_core
    PUBLIC
        ULWAI_VERSION_STRING="${ULWAI_VERSION}"
        ULWAI_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        ULWAI_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        ULWAI_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Main executable: ulw-ai-agent
set(APP_SOURCES src/main.cpp)

if(GUI_AVAILABLE)
    list(APPEND APP_SOURCES ${GUI_SOURCES} ${GUI_HEADERS})
endif()

add_executable(${ULWAI_PROJECT_NAME} ${APP_SOURCES})

target_link_libraries(${ULWAI_PROJECT_NAME}
    PRIVATE
        ulwai_core
)

if(GUI_AVAILABLE)
    set_target_properties(${ULWAI_PROJECT_NAME} PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )
    if(APPLE)
        set_target_properties(${ULWAI_PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "ULWAI AI Agent"
            MACOSX_BUNDLE_GUI_IDENTIFIER "in.eg1.ulwai"
            MACOSX_BUNDLE_BUNDLE_VERSION "${ULWAI_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${ULWAI_VERSION}"
        )
    endif()
    target_link_libraries(${ULWAI_PROJECT_NAME}
        PRIVATE
            ${QT_LIBRARIES}
    )
    target_compile_definitions(${ULWAI_PROJECT_NAME} PRIVATE ULWAI_ENABLE_GUI)
endif()

# Short command name executable: ulwai
add_executable(ulwai ${APP_SOURCES})
target_link_libraries(ulwai
    PRIVATE
        ulwai_core
)

if(GUI_AVAILABLE)
    set_target_properties(ulwai PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )
    target_link_libraries(ulwai
        PRIVATE
            ${QT_LIBRARIES}
    )
    target_compile_definitions(ulwai PRIVATE ULWAI_ENABLE_GUI)
endif()

# Standalone pure-terminal CLI binary: ulwai-cli (Zero Qt dependencies)
add_executable(ulwai-cli src/main_cli.cpp)
target_link_libraries(ulwai-cli
    PRIVATE
        ulwai_core
)

# Installation rules
install(TARGETS ${ULWAI_PROJECT_NAME} ulwai ulwai-cli
    RUNTIME DESTINATION bin
)

# ============================================================================
# Unit Tests
# ============================================================================

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
