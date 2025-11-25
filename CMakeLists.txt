cmake_minimum_required(VERSION 3.16)
project(UnderwaterIPC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------
# Compiler warnings & sanitizers (Debug)
# ---------------------------------------
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        add_compile_options(-Wall -Wextra -Wpedantic -fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wall -Wextra -Wpedantic -fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# ---------------------------------------
# Include global project headers
# ---------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/include)

# ---------------------------------------
# Find OpenCV
# ---------------------------------------
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV includes: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libs:     ${OpenCV_LIBS}")

# ---------------------------------------
# Find SQLite3
# ---------------------------------------
find_package(SQLite3 REQUIRED)
message(STATUS "SQLite3 include: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3 lib:     ${SQLite3_LIBRARIES}")

# ---------------------------------------
# ZeroMQ (Homebrew/macOS default)
# ---------------------------------------
if(APPLE)
    set(ZMQ_INCLUDE_DIR "/opt/homebrew/include")
    set(ZMQ_LIBRARY "/opt/homebrew/lib/libzmq.dylib")
endif()

# Create imported target for ZeroMQ
add_library(ZMQ::ZMQ STATIC IMPORTED)
set_target_properties(ZMQ::ZMQ PROPERTIES
    IMPORTED_LOCATION "${ZMQ_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ZMQ_INCLUDE_DIR}"
)

# ---------------------------------------
# Subdirectories
# ---------------------------------------
add_subdirectory(src/generator)
add_subdirectory(src/processor)
add_subdirectory(src/logger)


# -----------------------------
# Add tests
# -----------------------------
enable_testing()
add_subdirectory(tests)


# cmake_minimum_required(VERSION 3.16)
# project(UnderwaterIPC)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # ---------------------------------------------------------
# # ZeroMQ (Homebrew default prefix)
# # ---------------------------------------------------------
# if(APPLE)
#     set(ZMQ_INCLUDE_DIR "/opt/homebrew/include")
#     set(ZMQ_LIBRARY "/opt/homebrew/lib/libzmq.dylib")

#     if(NOT EXISTS "${ZMQ_INCLUDE_DIR}/zmq.hpp")
#         message(FATAL_ERROR "ZeroMQ header not found at ${ZMQ_INCLUDE_DIR}")
#     endif()

#     if(NOT EXISTS "${ZMQ_LIBRARY}")
#         message(FATAL_ERROR "ZeroMQ library not found at ${ZMQ_LIBRARY}")
#     endif()

#     message(STATUS "ZeroMQ include: ${ZMQ_INCLUDE_DIR}")
#     message(STATUS "ZeroMQ library: ${ZMQ_LIBRARY}")
# endif()

# # ---------------------------------------------------------
# # OpenCV detection
# # ---------------------------------------------------------
# find_package(OpenCV REQUIRED)
# message(STATUS "OpenCV includes: ${OpenCV_INCLUDE_DIRS}")
# message(STATUS "OpenCV libs:     ${OpenCV_LIBS}")

# # ---------------------------------------------------------
# # Provide paths to subdirectories
# # ---------------------------------------------------------
# set(ZMQ_INCLUDE_DIR ${ZMQ_INCLUDE_DIR} CACHE PATH "ZeroMQ include path")
# set(ZMQ_LIBRARY ${ZMQ_LIBRARY} CACHE FILEPATH "ZeroMQ library")

# include_directories(${OpenCV_INCLUDE_DIRS})
# include_directories(${ZMQ_INCLUDE_DIR})
# include_directories(${PROJECT_SOURCE_DIR}/include)

# # ---------------------------------------------------------
# # Add project modules
# # ---------------------------------------------------------
# add_subdirectory(src/generator)
# add_subdirectory(src/processor)
# add_subdirectory(src/logger)
