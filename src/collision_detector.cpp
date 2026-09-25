#include "mks_servo42c/collision_detector.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <thread>

namespace mks_servo42c {

// ============================================================
// Адаптивный порог (кусочно-линейная модель)
// ============================================================
double CollisionDetector::adaptive_threshold(const CollisionConfig& cfg,
                                             int speed) {
    if (speed <= 60) {
        return cfg.thr_low;
    } else if (speed <= 90) {
        return cfg.thr_low + (speed - 60) * cfg.thr_mid_k;
    } else {
        double thr_at_90 = cfg.thr_low + 30.0 * cfg.thr_mid_k;  // = 0.39
        return thr_at_90 + (speed - 90) * cfg.thr_high_k;
    }
}

// ============================================================
// Конструктор / деструктор
// ============================================================
CollisionDetector::CollisionDetector(Servo& servo, const CollisionConfig& config)
    : servo_(servo), config_(config) {}

CollisionDetector::~CollisionDetector() = default;

void CollisionDetector::request_stop() {
    stop_requested_ = true;
}

// ============================================================
// Реакция на столкновение
// ============================================================
void CollisionDetector::react_to_collision(const CollisionEvent& event) {
    std::cout << "\n*** COLLISION #" << events_.size() << " detected! ***\n";
    std::cout << "   avg_error:  " << event.avg_error_deg << " deg\n";
    std::cout << "   max_error:  " << event.max_error_deg << " deg\n";
    std::cout << "   threshold:  " << event.threshold_used << " deg\n";

    // 1. Остановить мотор
    std::cout << "   -> Stopping motor...\n";
    servo_.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 2. Отъехать назад
    std::cout << "   -> Retreating " << config_.retreat_pulses << " pulses...\n";
    Direction back_dir = (event.direction == Direction::CW)
                       ? Direction::CCW : Direction::CW;
    servo_.move(config_.retreat_speed, back_dir,
                static_cast<uint16_t>(config_.retreat_pulses));
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // 3. Пауза
    std::cout << "   -> Pause 1 second...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 4. Продолжить движение
    std::cout << "   -> Resuming motion...\n";
    servo_.run(event.speed, event.direction);

    // 5. Settle time — игнорировать ошибку, пока мотор стабилизируется
    std::cout << "   -> Settling (" << config_.settle_time_ms << " ms)...\n";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.settle_time_ms));
}

// ============================================================
// Главный цикл мониторинга
// ============================================================
void CollisionDetector::monitor(int speed, Direction dir, int duration_sec) {
    // Вычисляем порог для этой скорости
    double threshold = config_.use_adaptive_threshold
                     ? adaptive_threshold(config_, speed)
                     : config_.threshold_deg;

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "MONITORING: speed=" << speed
              << ", dir=" << (dir == Direction::CW ? "CW" : "CCW")
              << ", duration=" << duration_sec << "s\n";
    std::cout << "Threshold mode: "
              << (config_.use_adaptive_threshold ? "ADAPTIVE" : "FIXED") << "\n";
    std::cout << "Threshold for speed=" << speed << ": "
              << threshold << " deg\n";
    std::cout << "Window: " << config_.window_size
              << ", consecutive: " << config_.consecutive_hits << "\n";
    std::cout << std::string(60, '=') << "\n";

    events_.clear();
    stop_requested_ = false;

    // Включаем драйвер и стартуем
    servo_.enable(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    servo_.run(speed, dir);

    // Ждём разгона, чтобы ошибка стабилизировалась
    std::cout << "   -> Spinning up (waiting for settle)...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    std::deque<double> window;
    int hit_count = 0;
    double max_error_in_window = 0.0;
    int sample_index = 0;   // ← ДОБАВИЛИ: счётчик замеров

    auto start = std::chrono::steady_clock::now();

    while (!stop_requested_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= duration_sec) break;

        // ↓↓ ПРОПУСК ПЕРВЫХ N ЗАМЕРОВ (startup skip) ↓↓
        sample_index++;
        if (sample_index <= config_.startup_skip_samples) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.poll_interval_ms));
            continue;
        }
        // ↑↑ КОНЕЦ ПРОПУСКА ↑↑

        auto error_opt = servo_.read_angle_error();
        auto status_opt = servo_.read_shaft_status();

        if (!error_opt || !status_opt) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.poll_interval_ms));
            continue;
        }

        double err = *error_opt;
        uint8_t status = *status_opt;
        double abs_err = std::abs(err);

        window.push_back(abs_err);
        if (static_cast<int>(window.size()) > config_.window_size) {
            window.pop_front();
        }

        double avg = 0.0;
        for (double v : window) avg += v;
        avg /= static_cast<double>(window.size());

        max_error_in_window = std::max(max_error_in_window, abs_err);

        // Логируем (в консоль)
        printf("  error=%+7.3f  avg=%6.3f  thr=%6.3f  status=%s\n",
               err, avg, threshold,
               status == 1 ? "BLOCKED" : "free");

        // Логируем в CSV
        if (logger_) {
            logger_->log(err, avg, status, 0);
        }

        // Проверка столкновения
        bool collision = (avg > threshold) || (status == 1);
        if (collision) {
            hit_count++;
            printf("  [WARN] Resistance! (%d/%d)\n", hit_count,
                   config_.consecutive_hits);
        } else {
            hit_count = 0;
            max_error_in_window = 0.0;
        }

        if (hit_count >= config_.consecutive_hits) {
            CollisionEvent ev;
            ev.timestamp = std::chrono::system_clock::now();
            ev.avg_error_deg = avg;
            ev.max_error_deg = max_error_in_window;
            ev.shaft_status = status;
            ev.speed = speed;
            ev.direction = dir;
            ev.threshold_used = threshold;
            events_.push_back(ev);

            if (logger_) {
                logger_->log(err, avg, status, 1);
            }

            if (callback_) callback_(ev);

            react_to_collision(ev);

            hit_count = 0;
            window.clear();
            max_error_in_window = 0.0;
            sample_index = 0;   // ← сбрасываем skip после столкновения

            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.settle_time_ms));

            start = std::chrono::steady_clock::now();
            continue;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.poll_interval_ms));
    }

    servo_.stop();
    std::cout << "\nTotal collisions: " << events_.size() << "\n";
    std::cout << "Motor stopped.\n";
}

}  // namespace mks_servo42c