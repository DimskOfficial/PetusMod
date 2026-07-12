cmake_minimum_required(VERSION 3.21)

# Geode's toolchain sets the required C++ standard; declare a floor here.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PetusMod VERSION 1.0.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    src/Session.cpp
    src/DefaultLevels.cpp
    src/VerifyScreenshot.cpp
    src/Textboxes.cpp
    src/ModsPopup.cpp
    src/VersionCheck.cpp
    src/VersusTab.cpp
    src/VersusApi.cpp
    src/VersusLobbyPopup.cpp
    src/VersusCountdown.cpp
    src/VersusMatch.cpp
    src/VersusResults.cpp
)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Set the GEODE_SDK environment variable to your Geode SDK path.")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE src)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
