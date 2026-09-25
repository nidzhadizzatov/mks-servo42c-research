# Адаптивный порог + Settle Time для MKS SERVO42C

## Проблема

### Фиксированный порог не работает на высоких скоростях

При использовании фиксированного порога (0.4°):
- При speed=110 ошибка без нагрузки = 0.76° > 0.4°.
- Детектор постоянно срабатывает без причины.
- Робот не может работать на высоких скоростях.

### Каскадные срабатывания от отъезда

При использовании операции retreat (отъезд):
- После retreat мотор теряет позицию.
- Ошибка временно растёт до 0.3–0.5°.
- Это вызывает новое ложное срабатывание.
- Результат: 144 срабатывания за 60 секунд без касания.

### Стартовая ошибка разгона

При старте мотора:
- Мотор разгоняется 0.5–1.5 секунды.
- Ошибка временно растёт до 0.5–0.8°.
- Это вызывает ложное срабатывание в начале.

## Решение

### 1. Адаптивный порог

Кусочно-линейная модель на основе данных эксперимента 6.2:
threshold(speed) =
0.15° if speed <= 60
0.15 + (speed-60)0.008 if 60 < speed <= 90
0.39 + (speed-90)0.06 if speed > 90


Параметры:
- `thr_low = 0.15` — для стабильного режима (speed <= 60).
- `thr_mid_k = 0.008` — наклон в переходной зоне (60-90).
- `thr_high_k = 0.06` — наклон в перегруженной зоне (>90).

### 2. Post-collision settle

После отъезда — 1.5 секунды игнорирования ошибки:

```cpp
servo_.run(event.speed, event.direction);
std::this_thread::sleep_for(
    std::chrono::milliseconds(config_.settle_time_ms));

servo_.run(speed, dir);
std::this_thread::sleep_for(
    std::chrono::milliseconds(config_.settle_time_ms));

Результат: стартовая ошибка разгона устранена.

Результаты
Таблица сравнения порогов
Speed	Ошибка (avg)	Фикс. порог 0.4°	Адапт. порог	Запас	Ложных (адапт.)
30	0.05°	OK	0.15°	3.0x	0
70	0.08°	OK	0.23°	2.9x	0
110	0.76°	Ложные	1.59°	2.1x	0
Вывод: адаптивный порог даёт 0 ложных срабатываний на всех скоростях.

Startup settle
Speed	До startup settle	После startup settle
30	1 ложное	0
70	1 ложное	0
110	1 ложное	0
Вывод: startup settle устраняет ложные срабатывания в начале.

Post-collision settle
Режим	Столкновений за 60 с	Ложных
Без settle time	144	100%
С settle time	0	0%
Вывод: settle time устраняет каскадные срабатывания.

Три уровня защиты
Механизм	Где работает	Что предотвращает
Адаптивный порог	По скорости	Ложные на высоких скоростях
Startup settle	При старте (1.5 с)	Ошибка разгона
Post-collision settle	После отъезда (1.5 с)	Каскадные срабатывания
Вместе они дают 0 ложных срабатываний на всех скоростях от 30 до 110.

Параметры конфигурации
cpp
CollisionConfig config;

// Адаптивный порог
config.use_adaptive_threshold = true;
config.thr_low    = 0.15;
config.thr_mid_k  = 0.008;
config.thr_high_k = 0.06;

// Settle time
config.settle_time_ms = 1500;  // 1.5 секунды

// Окно и счётчик
config.window_size = 5;
config.consecutive_hits = 3;
config.poll_interval_ms = 50;

// Отъезд
config.retreat_pulses = 400;   // 1/8 оборота
config.retreat_speed = 30;

// Startup skip (опционально)
config.startup_skip_samples = 20;
Ключевые выводы
Фиксированный порог 0.4° работает только до speed=90.

На speed=110 фиксированный порог даёт 100% ложных срабатываний.

Адаптивный порог даёт запас 2–3x на всех скоростях.

Settle time (1.5 с) полностью устраняет каскадные срабатывания.

Комбинация трёх механизмов даёт 0 ложных срабатываний.

Данные
data/logs/calibration_v3.csv — калибровка скорости

data/logs/following_error.csv — следящая ошибка

data/logs/adaptive_speed30.log — тест на speed=30

data/logs/adaptive_speed70.log — тест на speed=70

data/logs/adaptive_speed110.log — тест на speed=110

Ссылки
Калибровка скорости: docs/calibration.md

Следящая ошибка: docs/following_error.md

Исходный код: src/collision_detector.cpp

API: include/mks_servo42c/collision_detector.hpp

text

## 🚀 Что делать

### 1. Создайте файл

**Путь:** `docs/adaptive_threshold_results.md`

Скопируйте **весь текст выше** (от `# Адаптивный порог...` до последней строки) и вставьте в файл.

### 2. Проверьте, что файл целый

```powershell
Get-Content docs\adaptive_threshold_results.md | Measure-Object -Line
Должно показать ~130–150 строк.