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
    double threshold_deg = 0.4;     // порог средней |ошибки|
    int window_size = 5;            // окно усреднения
    int consecutive_hits = 3;       // сколько подряд нужно для тревоги
    int poll_interval_ms = 50;      // интервал опроса
    int retreat_pulses = 400;       // отъезд при столкновении
    int retreat_speed = 30;         // скорость отъезда
};

struct CollisionEvent {
    std::chrono::system_clock::time_point timestamp;
    double avg_error_deg = 0.0;
    double max_error_deg = 0.0;
    uint8_t shaft_status = 0;
    int speed = 0;
    Direction direction = Direction::CW;
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
    void set_logger(Logger* logger) { logger_ = logger; }
    
private:
    Servo& servo_;
    CollisionConfig config_;
    std::vector<CollisionEvent> events_;
    EventCallback callback_;
    bool stop_requested_ = false;
    
    void react_to_collision(const CollisionEvent& event);
    Logger* logger_ = nullptr;
    
};

}  // namespace mks_servo42c