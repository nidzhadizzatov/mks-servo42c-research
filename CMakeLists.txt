cmake_minimum_required(VERSION 3.15)
project(mks_servo42c_research CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mks_servo42c STATIC
    src/serial_port.cpp
    src/protocol.cpp
    src/servo.cpp
    src/collision_detector.cpp
    src/logger.cpp
)
target_include_directories(mks_servo42c PUBLIC include/)

add_executable(01_serial_test         examples/01_serial_test.cpp)
add_executable(02_read_params         examples/02_read_params.cpp)
add_executable(03_raw_pulse           examples/03_raw_pulse.cpp)
add_executable(04_test_motion         examples/04_test_motion.cpp)
add_executable(05_collision_detection examples/05_collision_detection.cpp)
add_executable(06_data_logger         examples/06_data_logger.cpp)
add_executable(07_speed_calibration examples/07_speed_calibration.cpp)
add_executable(08_following_error examples/08_following_error.cpp)
add_executable(09_adaptive_threshold examples/09_adaptive_threshold.cpp)

target_link_libraries(01_serial_test         mks_servo42c)
target_link_libraries(02_read_params         mks_servo42c)
target_link_libraries(03_raw_pulse           mks_servo42c)
target_link_libraries(04_test_motion         mks_servo42c)
target_link_libraries(05_collision_detection mks_servo42c)
target_link_libraries(06_data_logger         mks_servo42c)
target_link_libraries(07_speed_calibration mks_servo42c)
target_link_libraries(08_following_error mks_servo42c)
target_link_libraries(09_adaptive_threshold mks_servo42c)

if (UNIX AND NOT APPLE)
    target_link_libraries(mks_servo42c PUBLIC pthread)
endif()