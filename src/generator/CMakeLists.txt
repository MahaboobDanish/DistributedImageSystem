add_executable(generator main.cpp)

# Include directories
target_include_directories(generator PRIVATE ${OpenCV_INCLUDE_DIRS})

# Link libraries
target_link_libraries(generator PRIVATE ZMQ::ZMQ ${OpenCV_LIBS})

# # Example for generator
# add_executable(generator main.cpp)

# # Include ZeroMQ headers
# target_include_directories(generator PRIVATE ${ZMQ_INCLUDE_DIR_FOUND})

# # Link ZeroMQ library
# target_link_libraries(generator PRIVATE ${ZMQ_LIBRARY_FOUND})

# # Link OpenCV if found
# if(OpenCV_FOUND)
#     target_include_directories(generator PRIVATE ${OpenCV_INCLUDE_DIRS})
#     target_link_libraries(generator PRIVATE ${OpenCV_LIBS})
# endif()
