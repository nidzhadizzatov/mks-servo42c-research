#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include "mks_servo42c/collision_detector.hpp"

#include <cstdio>
#include <iostream>
#include <string>

using namespace mks_servo42c;

// Печатает таблицу порогов для разных скоростей
void print_threshold_table(const CollisionConfig& cfg) {
    std::cout << "\n=== Adaptive Threshold Table ===\n";
    std::printf("%-8s %-12s\n", "Speed", "Threshold");
    for (int s = 10; s <= 120; s += 10) {
        double t = CollisionDetector::adaptive_threshold(cfg, s);
        std::printf("%-8d %-12.3f\n", s, t);
    }
    std::cout << "\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [speed] [duration]\n";
        std::cerr << "  port     — COM7 (Windows) or /dev/ttyUSB0 (Linux)\n";
        std::cerr << "  speed    — 10..120 (default 30)\n";
        std::cerr << "  duration — seconds (default 30)\n";
        return 1;
    }

    std::string port_name = argv[1];
    int speed    = (argc > 2) ? std::stoi(argv[2]) : 30;
    int duration = (argc > 3) ? std::stoi(argv[3]) : 30;

    SerialPort port;
    if (!port.open(port_name, 38400)) {
        std::cerr << "Failed to open " << port_name << "\n";
        return 1;
    }

    Servo servo(port);

    CollisionConfig config;
    // Адаптивный порог включён по умолчанию
    config.use_adaptive_threshold = true;
    config.thr_low    = 0.15;
    config.thr_mid_k  = 0.008;
    config.thr_high_k = 0.06;
    config.window_size = 5;
    config.consecutive_hits = 3;
    config.poll_interval_ms = 50;
    config.retreat_pulses = 400;
    config.retreat_speed = 30;
    config.settle_time_ms = 1500; 

    // Показываем таблицу порогов
    print_threshold_table(config);

    CollisionDetector detector(servo, config);

    std::cout << "Starting adaptive threshold test...\n";
    std::cout << "Speed: " << speed << ", Duration: " << duration << "s\n";
    std::cout << "Grab the motor shaft to trigger a collision.\n\n";

    detector.monitor(speed, Direction::CW, duration);

    port.close();
    return 0;
}