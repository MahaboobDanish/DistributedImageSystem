cmake_minimum_required(VERSION 3.16)
project(UnderwaterIPC_Tests)

# GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/../include)

# OpenCV for tests
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# -----------------------------
# Unit tests
# -----------------------------
add_executable(unit_ipc_utils unit/ipc_utils_test.cpp)
target_link_libraries(unit_ipc_utils PRIVATE GTest::gtest_main ${OpenCV_LIBS})
add_test(NAME ipc_utils_test COMMAND unit_ipc_utils)

# -----------------------------
# E2E tests
# -----------------------------
add_executable(e2e_flow e2e/e2e_flow_test.cpp)
target_link_libraries(e2e_flow PRIVATE GTest::gtest_main ${OpenCV_LIBS})
add_test(NAME e2e_flow_test COMMAND e2e_flow)
