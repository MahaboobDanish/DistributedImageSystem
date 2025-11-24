add_executable(logger main.cpp)

# Include directories
target_include_directories(logger PRIVATE ${OpenCV_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})

# Link libraries
target_link_libraries(logger PRIVATE ZMQ::ZMQ ${OpenCV_LIBS} ${SQLite3_LIBRARIES})

# # Example for generator
# add_executable(logger main.cpp)

# # Include ZeroMQ headers
# target_include_directories(logger PRIVATE ${ZMQ_INCLUDE_DIR_FOUND})

# # Link ZeroMQ library
# target_link_libraries(logger PRIVATE ${ZMQ_LIBRARY_FOUND})

# # Link OpenCV if found
# if(OpenCV_FOUND)
#     target_include_directories(logger PRIVATE ${OpenCV_INCLUDE_DIRS})
#     target_link_libraries(logger PRIVATE ${OpenCV_LIBS})
# endif()
