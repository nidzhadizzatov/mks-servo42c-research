#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include "mks_servo42c/collision_detector.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [speed] [duration]\n";
        std::cerr << "  port     — COM7 (Windows) or /dev/ttyUSB0 (Linux)\n";
        std::cerr << "  speed    — 0..127 (default 30)\n";
        std::cerr << "  duration — seconds (default 30)\n";
        return 1;
    }

    std::string port_name = argv[1];
    int speed = (argc > 2) ? std::stoi(argv[2]) : 30;
    int duration = (argc > 3) ? std::stoi(argv[3]) : 30;

    mks_servo42c::SerialPort port;
    if (!port.open(port_name, 38400)) {
        std::cerr << "Failed to open " << port_name << "\n";
        return 1;
    }

    mks_servo42c::Servo servo(port);

    mks_servo42c::CollisionConfig config;
    config.threshold_deg = 0.4;
    config.window_size = 5;
    config.consecutive_hits = 3;
    config.poll_interval_ms = 50;
    config.retreat_pulses = 400;
    config.retreat_speed = 30;

    mks_servo42c::CollisionDetector detector(servo, config);

    std::cout << "Starting collision detection...\n";
    std::cout << "Grab the motor shaft to trigger a collision.\n";

    detector.monitor(speed, mks_servo42c::Direction::CW, duration);

    port.close();
    return 0;
}