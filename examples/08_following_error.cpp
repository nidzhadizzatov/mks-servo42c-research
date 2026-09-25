#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace mks_servo42c;

struct ErrorStats {
    int speed;
    double avg_error;
    double max_error;
    double std_error;
    int samples;
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <output.csv>\n";
        return 1;
    }

    std::string port_name = argv[1];
    std::string output_csv = argv[2];

    SerialPort port;
    if (!port.open(port_name, 38400)) {
        std::cerr << "Failed to open " << port_name << "\n";
        return 1;
    }

    Servo servo(port);

    std::vector<int> speeds = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120};

    FILE* csv = std::fopen(output_csv.c_str(), "w");
    if (!csv) {
        std::cerr << "Failed to open " << output_csv << "\n";
        return 1;
    }
    std::fprintf(csv, "speed,avg_error,max_error,std_error,samples\n");
    std::fflush(csv);

    std::cout << "=== Following Error Test ===\n";
    std::cout << "Measuring |angle_error| at various speeds WITHOUT load\n\n";

    servo.enable(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::vector<ErrorStats> results;

    for (int speed : speeds) {
        std::cout << "Testing speed " << speed << "... " << std::flush;

        // Запускаем мотор
        servo.run(speed, Direction::CW);
        std::this_thread::sleep_for(std::chrono::seconds(2));  // разгон

        // Собираем 100 замеров ошибки
        std::vector<double> errors;
        errors.reserve(100);

        for (int i = 0; i < 100; ++i) {
            auto err = servo.read_angle_error();
            if (err) {
                errors.push_back(std::abs(*err));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        // Останавливаем
        servo.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        if (errors.empty()) {
            std::cout << "FAIL (no data)\n";
            continue;
        }

        // Считаем статистику
        double sum = 0.0, max_e = 0.0;
        for (double e : errors) {
            sum += e;
            if (e > max_e) max_e = e;
        }
        double avg = sum / errors.size();

        double var_sum = 0.0;
        for (double e : errors) {
            var_sum += (e - avg) * (e - avg);
        }
        double stddev = std::sqrt(var_sum / errors.size());

        ErrorStats stats{speed, avg, max_e, stddev, static_cast<int>(errors.size())};
        results.push_back(stats);

        std::printf("avg=%.4f°  max=%.4f°  std=%.4f°  (n=%d)\n",
                    avg, max_e, stddev, stats.samples);

        std::fprintf(csv, "%d,%.4f,%.4f,%.4f,%d\n",
                     speed, avg, max_e, stddev, stats.samples);
        std::fflush(csv);
    }

    std::fclose(csv);

    std::cout << "\n=== Summary ===\n";
    std::printf("%-8s %-12s %-12s %-12s\n", "Speed", "Avg|err|", "Max|err|", "Std");
    for (const auto& r : results) {
        std::printf("%-8d %-12.4f %-12.4f %-12.4f\n",
                    r.speed, r.avg_error, r.max_error, r.std_error);
    }

    // Предложение адаптивного порога
    std::cout << "\n=== Suggested Adaptive Threshold ===\n";
    std::cout << "threshold(speed) = base + k * (speed / 120)\n";
    if (!results.empty()) {
        double max_avg = results.back().avg_error;
        double base = results.front().avg_error;
        std::cout << "base = " << base << "°\n";
        std::cout << "k = " << (max_avg - base) << "°\n";
        std::cout << "Example: threshold(60) = " << (base + (max_avg - base) * 0.5) << "°\n";
    }

    std::cout << "\nSaved to " << output_csv << "\n";

    servo.enable(false);
    port.close();
    return 0;
}