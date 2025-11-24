add_executable(processor main.cpp)

# Include directories
target_include_directories(processor PRIVATE ${OpenCV_INCLUDE_DIRS})

# Link libraries
target_link_libraries(processor PRIVATE ZMQ::ZMQ ${OpenCV_LIBS})

# # Example for generator
# add_executable(processor main.cpp)

# # Include ZeroMQ headers
# target_include_directories(processor PRIVATE ${ZMQ_INCLUDE_DIR_FOUND})

# # Link ZeroMQ library
# target_link_libraries(processor PRIVATE ${ZMQ_LIBRARY_FOUND})

# # Link OpenCV if found
# if(OpenCV_FOUND)
#     target_include_directories(processor PRIVATE ${OpenCV_INCLUDE_DIRS})
#     target_link_libraries(processor PRIVATE ${OpenCV_LIBS})
# endif()
