#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include "mks_servo42c/collision_detector.hpp"
#include "mks_servo42c/timing_logger.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <deque>
#include <iostream>
#include <string>
#include <thread>

using namespace mks_servo42c;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <port> <speed> <duration_sec> [output.csv]\n";
        return 1;
    }

    std::string port_name = argv[1];
    int speed = std::stoi(argv[2]);
    int duration = std::stoi(argv[3]);
    std::string output_csv = (argc > 4) ? argv[4] : "detection_time.csv";

    SerialPort port;
    if (!port.open(port_name, 38400)) {
        std::cerr << "Failed to open " << port_name << "\n";
        return 1;
    }

    Servo servo(port);
    TimingLogger logger;
    if (!logger.open(output_csv)) {
        std::cerr << "Failed to open " << output_csv << "\n";
        return 1;
    }

    CollisionConfig config;
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

    double threshold = CollisionDetector::adaptive_threshold(config, speed);

    std::cout << "\n=== Detection Time Measurement ===\n";
    std::cout << "Speed: " << speed << "\n";
    std::cout << "Threshold: " << threshold << " deg\n";
    std::cout << "Duration: " << duration << " s\n";
    std::cout << "Output: " << output_csv << "\n\n";

    servo.enable(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    servo.run(speed, Direction::CW);

    std::cout << "Spinning up (settle " << config.settle_time_ms << " ms)...\n";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config.settle_time_ms));

    std::deque<double> window;
    int hit_count = 0;
    double max_error_in_window = 0.0;

    auto start = std::chrono::steady_clock::now();
    auto last_free_time = start;
    double detection_time_ms = -1.0;
    bool in_collision = false;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - start).count();
        if (elapsed >= duration) break;

        auto error_opt = servo.read_angle_error();
        auto status_opt = servo.read_shaft_status();

        if (!error_opt || !status_opt) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.poll_interval_ms));
            continue;
        }

        double err = *error_opt;
        uint8_t status = *status_opt;
        double abs_err = std::abs(err);

        window.push_back(abs_err);
        if (static_cast<int>(window.size()) > config.window_size) {
            window.pop_front();
        }

        double avg = 0.0;
        for (double v : window) avg += v;
        avg /= static_cast<double>(window.size());

        max_error_in_window = std::max(max_error_in_window, abs_err);

        bool collision_detected = (avg > threshold) || (status == 1);

        if (collision_detected) {
            hit_count++;
            if (!in_collision) {
                auto t_now = std::chrono::steady_clock::now();
                detection_time_ms = std::chrono::duration<double, std::milli>(
                    t_now - last_free_time).count();
                in_collision = true;
                std::cout << "\n*** COLLISION detected! ***\n";
                std::printf("   Detection time: %.1f ms\n", detection_time_ms);
            }
        } else {
            hit_count = 0;
            last_free_time = std::chrono::steady_clock::now();
            in_collision = false;
        }

        // Логируем КАЖДЫЙ замер
        logger.log(err, avg, status,
                   collision_detected ? 1 : 0,
                   speed, detection_time_ms);

        // Вывод
        auto t_ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
        printf("  t=%.0fms  error=%+7.3f  avg=%6.3f  status=%s  %s\n",
               t_ms, err, avg,
               status == 1 ? "BLOCKED" : "free",
               collision_detected ? "COLLISION" : "");

        // Реакция
        if (hit_count >= config.consecutive_hits) {
            std::cout << "   -> Reacting to collision...\n";
            servo.stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            Direction back_dir = (Direction::CW == Direction::CCW)
                               ? Direction::CCW : Direction::CW;
            servo.move(config.retreat_speed, back_dir,
                       static_cast<uint16_t>(config.retreat_pulses));
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));

            servo.run(speed, Direction::CW);
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config.settle_time_ms));

            hit_count = 0;
            window.clear();
            max_error_in_window = 0.0;
            in_collision = false;
            detection_time_ms = -1.0;
            last_free_time = std::chrono::steady_clock::now();
            start = std::chrono::steady_clock::now();
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(config.poll_interval_ms));
    }

    servo.stop();
    servo.enable(false);
    logger.close();
    port.close();

    std::cout << "\n=== Done ===\n";
    std::cout << "Data saved to " << output_csv << "\n";
    return 0;
}