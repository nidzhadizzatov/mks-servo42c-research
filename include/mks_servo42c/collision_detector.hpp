#pragma once

#include "mks_servo42c/servo.hpp"
#include "mks_servo42c/logger.hpp"

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <vector>

namespace mks_servo42c {

struct CollisionConfig {
    // Основные параметры
    int window_size = 5;            // окно усреднения
    int consecutive_hits = 3;       // сколько подряд для тревоги
    int poll_interval_ms = 50;      // интервал опроса
    int retreat_pulses = 400;       // отъезд при столкновении
    int retreat_speed = 30;         // скорость отъезда
    int settle_time_ms = 1500;
    // --- Порог: фиксированный или адаптивный ---
    bool use_adaptive_threshold = true;

    // Фиксированный порог (используется если use_adaptive_threshold = false)
    double threshold_deg = 0.4;

    // --- Адаптивный порог (кусочно-линейная модель) ---
    // На основе эксперимента 6.2: speed -> avg|error|
    //
    //   speed <= 60:        threshold = thr_low
    //   60 < speed <= 90:   threshold = thr_low + (speed-60) * thr_mid_k
    //   speed > 90:         threshold = thr_at_90 + (speed-90) * thr_high_k
    //
    double thr_low    = 0.15;   // порог на speed <= 60
    double thr_mid_k  = 0.008;  // наклон в переходной зоне (60-90)
    double thr_high_k = 0.06;   // наклон в перегруженной зоне (>90)
};

struct CollisionEvent {
    std::chrono::system_clock::time_point timestamp;
    double avg_error_deg = 0.0;
    double max_error_deg = 0.0;
    uint8_t shaft_status = 0;
    int speed = 0;
    Direction direction = Direction::CW;
    double threshold_used = 0.0;  // какой порог сработал
};

class CollisionDetector {
public:
    using EventCallback = std::function<void(const CollisionEvent&)>;

    CollisionDetector(Servo& servo, const CollisionConfig& config = {});
    ~CollisionDetector();

    // Запуск наблюдения. Блокирует поток на duration_sec секунд.
    void monitor(int speed, Direction dir, int duration_sec);

    // Остановить досрочно (из другого потока).
    void request_stop();

    // Получить все зафиксированные события.
    const std::vector<CollisionEvent>& events() const { return events_; }

    // Установить колбэк, вызываемый при каждом столкновении.
    void set_callback(EventCallback cb) { callback_ = std::move(cb); }

    // Установить логгер (опционально).
    void set_logger(Logger* logger) { logger_ = logger; }

    // Утилита: вычислить порог для скорости (публичная, для тестов).
    static double adaptive_threshold(const CollisionConfig& cfg, int speed);

private:
    Servo& servo_;
    CollisionConfig config_;
    std::vector<CollisionEvent> events_;
    EventCallback callback_;
    Logger* logger_ = nullptr;
    bool stop_requested_ = false;

    void react_to_collision(const CollisionEvent& event);
};

}  // namespace mks_servo42c