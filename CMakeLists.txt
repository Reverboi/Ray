cmake_minimum_required(VERSION 3.15)
project(Ray)

set(CMAKE_CXX_STANDARD 17)

# Enable pkg-config
find_package(PkgConfig REQUIRED)

# Query pkg-config for dependencies
pkg_check_modules(NCURSES REQUIRED ncurses)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)
pkg_check_modules(LIBUDEV REQUIRED libudev)

# Gather source files
file(GLOB SOURCES src/*.cpp)

# Define your executable
add_executable(Ray ${SOURCES})

# Add include directories
target_include_directories(Ray PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${NCURSES_INCLUDE_DIRS}
    ${LIBEVDEV_INCLUDE_DIRS}
    ${LIBUDEV_INCLUDE_DIRS}
)

# Link against the libraries
target_link_libraries(Ray PRIVATE
    ${NCURSES_LIBRARIES}
    ${LIBEVDEV_LIBRARIES}
    ${LIBUDEV_LIBRARIES}
)
