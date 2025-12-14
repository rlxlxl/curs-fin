# Многоэтапная сборка для уменьшения размера образа
FROM ubuntu:22.04 AS builder

# Установка зависимостей для сборки
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    libpq-dev \
    make \
    && rm -rf /var/lib/apt/lists/*

# Установка рабочей директории
WORKDIR /app

# Копирование файлов проекта
COPY Makefile ./
COPY src/ ./src/
COPY include/ ./include/
COPY sql/ ./sql/

# Сборка проекта
# В Docker используем Linux пути (Makefile автоматически определит Linux)
RUN make clean && make

# Финальный образ
FROM ubuntu:22.04

# Установка только runtime зависимостей
RUN apt-get update && apt-get install -y \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

# Создание пользователя для запуска приложения
RUN useradd -m -u 1000 appuser

WORKDIR /app

# Копирование скомпилированного приложения и SQL файлов
COPY --from=builder /app/build/server /app/server
COPY --from=builder /app/sql/ /app/sql/

# Изменение владельца файлов
RUN chown -R appuser:appuser /app

# Переключение на непривилегированного пользователя
USER appuser

# Открытие порта
EXPOSE 8080

# Запуск приложения
CMD ["./server"]

