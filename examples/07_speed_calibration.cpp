#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include <cmath>

#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace mks_servo42c;

struct CalibrationResult {
    int speed;
    int32_t pulse_start;
    int32_t pulse_end;
    double duration_sec;
    double rpm;
};

// Читаем pulse_count (накопительный, не оборачивается)
std::optional<int32_t> read_pulse(Servo& servo) {
    return servo.read_pulse_count();
}

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
    std::fprintf(csv, "speed,pulse_start,pulse_end,delta_pulses,duration_sec,rpm\n");
    std::fflush(csv);

    std::cout << "=== Speed Calibration v2 (using pulse_count) ===\n";
    std::cout << "Speeds: ";
    for (int s : speeds) std::cout << s << " ";
    std::cout << "\n\n";

    servo.enable(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    constexpr double STEPS_PER_REV = 3200.0;  // 16 микрошагов × 200 шагов
    std::vector<CalibrationResult> results;

    for (int speed : speeds) {
        std::cout << "Testing speed " << speed << "... " << std::flush;

        // 1. Запускаем мотор
        servo.run(speed, Direction::CW);
        std::this_thread::sleep_for(std::chrono::seconds(2));  // разгон

        // 2. Читаем начальный pulse_count (усредняем 3 раза для стабильности)
        int32_t p_start = 0;
        for (int i = 0; i < 3; ++i) {
            auto p = read_pulse(servo);
            if (p) p_start = *p;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        // 3. Короткий измерительный интервал (1 секунда)
        auto t_start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto t_end = std::chrono::steady_clock::now();
        double duration = std::chrono::duration<double>(t_end - t_start).count();

        // 4. Читаем конечный pulse_count
        int32_t p_end = 0;
        for (int i = 0; i < 3; ++i) {
            auto p = read_pulse(servo);
            if (p) p_end = *p;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        // 5. Останавливаем
        servo.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        // 6. Считаем RPM
        int32_t delta = p_end - p_start;
        double delta_rev = static_cast<double>(delta) / STEPS_PER_REV;
        double rpm = std::abs(delta_rev / duration) * 60.0;

        CalibrationResult r{speed, p_start, p_end, duration, rpm};
        results.push_back(r);

        std::printf("RPM = %8.2f (delta_pulses = %d, rev = %.2f)\n",
                    rpm, delta, delta_rev);

        std::fprintf(csv, "%d,%d,%d,%d,%.3f,%.3f\n",
                     speed, p_start, p_end, delta, duration, rpm);
        std::fflush(csv);
    }

    std::fclose(csv);

    std::cout << "\n=== Summary ===\n";
    std::printf("%-8s %-12s %-12s\n", "Speed", "RPM", "Rev/sec");
    for (const auto& r : results) {
        double delta_rev = static_cast<double>(r.pulse_end - r.pulse_start) / STEPS_PER_REV;
        std::printf("%-8d %-12.2f %-12.3f\n",
                    r.speed, r.rpm, delta_rev / r.duration_sec);
    }

    std::cout << "\nSaved to " << output_csv << "\n";

    servo.enable(false);
    port.close();
    return 0;
}