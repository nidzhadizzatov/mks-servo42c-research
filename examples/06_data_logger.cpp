#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include "mks_servo42c/collision_detector.hpp"
#include "mks_servo42c/logger.hpp"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [speed] [duration] [output.csv]\n";
        return 1;
    }

    std::string port_name = argv[1];
    int speed    = (argc > 2) ? std::stoi(argv[2]) : 30;
    int duration = (argc > 3) ? std::stoi(argv[3]) : 30;
    std::string output = (argc > 4) ? argv[4] : "experiment_log.csv";

    mks_servo42c::SerialPort port;
    if (!port.open(port_name, 38400)) {
        std::cerr << "Failed to open " << port_name << "\n";
        return 1;
    }

    mks_servo42c::Servo servo(port);

    mks_servo42c::Logger logger;
    if (!logger.open(output)) {
        std::cerr << "Failed to open log file " << output << "\n";
        return 1;
    }
    std::cout << "Logging to " << output << "\n";

    mks_servo42c::CollisionConfig config;
    config.threshold_deg = 0.4;
    config.window_size = 5;
    config.consecutive_hits = 3;
    config.poll_interval_ms = 50;
    config.retreat_pulses = 400;
    config.retreat_speed = 30;

    mks_servo42c::CollisionDetector detector(servo, config);
    detector.set_logger(&logger);

    std::cout << "Starting experiment: speed=" << speed
              << ", duration=" << duration << "s\n";
    std::cout << "Grab the motor shaft to trigger collisions.\n";

    detector.monitor(speed, mks_servo42c::Direction::CW, duration);

    logger.close();
    port.close();
    std::cout << "\nExperiment done. Data saved to " << output << "\n";
    return 0;
}