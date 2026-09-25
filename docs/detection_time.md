# Detection Time для MKS SERVO42C

## Методика

- Мотор: MKS SERVO42C (замкнутый шаговый)
- Скорость: 30
- Порог: 0.15° (адаптивный)
- Окно: 5 замеров
- Consecutive hits: 3
- Poll interval: 50 мс
- Нагрузка: рука оператора

## Результаты

| Событие | Detection Time |
|---------|----------------|
| #1 | 61.6 мс |
| #2 | 124.7 мс |
| #3 | 126.1 мс |
| #4 | 140.5 мс |
| #5 | 139.3 мс |
| #6 | 122.9 мс |

**Среднее:** ~119 мс
**Мин:** 61.6 мс
**Макс:** 140.5 мс

## Анализ

### Формула

Detection time ≈ poll_interval × (window_size + consecutive_hits)
= 50 мс × (5 + 3) = **400 мс** (теоретический максимум)

**Практически:** 119 мс — быстрее, потому что при резком росте ошибки avg пересекает порог раньше.

### Сравнение с литературой

| Метод | Detection Time |
|-------|----------------|
| Momentum observer | 40–130 мс |
| Extended state observer | 50–100 мс |
| **Наш метод** | **119 мс** |

**Вывод:** наш метод в диапазоне литературы, но **не требует** датчика усилия или внешней модели динамики.

### Оптимизация

Detection time можно уменьшить:
- `poll_interval_ms = 20` → detection ~50 мс.
- `window_size = 3` → быстрее, но больше ложных.
- `consecutive_hits = 2` → быстрее, но менее надёжно.

## График

![Detection Time](data/logs/detection_time_speed30.png)

- **Слева:** error vs time, 6 столкновений.
- **Справа:** collision events.

## Данные

- `data/logs/detection_time_speed30.csv`
- `data/logs/detection_time_speed30.png`