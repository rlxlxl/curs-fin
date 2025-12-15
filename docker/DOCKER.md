# Docker инструкции

## Быстрый старт

### Запуск с Docker Compose (рекомендуется)

1. Убедитесь, что Docker и Docker Compose установлены:
```bash
docker --version
docker-compose --version
```

2. Запустите проект (из папки docker):
```bash
cd docker
docker-compose up --build
```

Или из корня проекта:
```bash
docker-compose -f docker/docker-compose.yml up --build
```

3. Откройте браузер:
```
http://localhost:8080
```

4. Войдите с учетными данными:
   - **Администратор**: `admin` / `admin123`
   - Или зарегистрируйтесь, нажав "Нет аккаунта? Зарегистрироваться" на странице входа

### Остановка

```bash
# Из папки docker
docker-compose down

# Или из корня проекта
docker-compose -f docker/docker-compose.yml down
```

### Остановка с удалением данных

```bash
# Из папки docker
docker-compose down -v

# Или из корня проекта
docker-compose -f docker/docker-compose.yml down -v
```

## Отдельные команды

### Сборка образа

```bash
# Из корня проекта
docker build -f docker/Dockerfile -t infosec-server .

# Или из папки docker
cd docker
docker build -f Dockerfile -t infosec-server ..
```

### Запуск контейнера с приложением

```bash
docker run -d \
  --name infosec_web \
  -p 8080:8080 \
  -e DB_HOST=postgres \
  -e DB_PORT=5432 \
  -e DB_NAME=infosec_db \
  -e DB_USER=postgres \
  -e DB_PASSWORD=password \
  --network infosec_network \
  infosec-server
```

### Запуск PostgreSQL

```bash
docker run -d \
  --name infosec_postgres \
  -e POSTGRES_DB=infosec_db \
  -e POSTGRES_USER=postgres \
  -e POSTGRES_PASSWORD=password \
  -p 5432:5432 \
  -v postgres_data:/var/lib/postgresql/data \
  postgres:15-alpine
```

## Переменные окружения

### Приложение (web)

- `DB_HOST` - хост PostgreSQL (по умолчанию: `localhost`)
- `DB_PORT` - порт PostgreSQL (по умолчанию: `5432`)
- `DB_NAME` - имя базы данных (по умолчанию: `infosec_db`)
- `DB_USER` - пользователь БД (по умолчанию: `postgres`)
- `DB_PASSWORD` - пароль БД (по умолчанию: `password`)

### PostgreSQL

- `POSTGRES_DB` - имя базы данных
- `POSTGRES_USER` - пользователь
- `POSTGRES_PASSWORD` - пароль

## Просмотр логов

```bash
# Из папки docker
# Логи всех сервисов
docker-compose logs

# Логи только веб-сервера
docker-compose logs web

# Логи только PostgreSQL
docker-compose logs postgres

# Следить за логами в реальном времени
docker-compose logs -f

# Или из корня проекта
docker-compose -f docker/docker-compose.yml logs
```

## Подключение к базе данных

```bash
# Через docker-compose (из папки docker)
docker-compose exec postgres psql -U postgres -d infosec_db

# Или из корня проекта
docker-compose -f docker/docker-compose.yml exec postgres psql -U postgres -d infosec_db

# Или напрямую
docker exec -it infosec_postgres psql -U postgres -d infosec_db
```

## Пересборка после изменений

```bash
# Из папки docker
# Пересборка и перезапуск
docker-compose up --build

# Только пересборка без запуска
docker-compose build

# Или из корня проекта
docker-compose -f docker/docker-compose.yml up --build
```

## Функциональность приложения

После запуска приложения доступны следующие возможности:

- **Регистрация** - отдельная страница `/register` с валидацией данных
- **Авторизация** - вход в систему с проверкой учетных данных
- **Поиск и фильтрация** - поиск интеграторов по названию и городу
- **Сортировка** - по названию, городу, рейтингу
- **Пагинация** - навигация по страницам результатов
- **Рейтинги и отзывы** - оценка интеграторов и комментарии
- **Административная панель** - управление каталогом (только для admin)

## Структура Docker файлов

```
.
├── docker/
│   ├── Dockerfile         # Образ для сборки и запуска приложения
│   ├── docker-compose.yml # Конфигурация для запуска всех сервисов
│   └── DOCKER.md          # Эта инструкция
├── .dockerignore          # Файлы, исключаемые из образа
└── sql/
    ├── init.sql           # SQL скрипт для инициализации БД
    ├── drop_all.sql       # Скрипт удаления всех таблиц
    └── reset_database.sql # Скрипт полного сброса БД
```

## Устранение проблем

### Порт 8080 уже занят

Измените порт в `docker/docker-compose.yml`:
```yaml
ports:
  - "8081:8080"  # Внешний порт:внутренний порт
```

### Порт 5432 уже занят

Измените порт PostgreSQL в `docker/docker-compose.yml`:
```yaml
postgres:
  ports:
    - "5433:5432"
```

### Ошибка подключения к БД

1. Проверьте, что PostgreSQL контейнер запущен:
```bash
# Из папки docker
docker-compose ps

# Или из корня проекта
docker-compose -f docker/docker-compose.yml ps
```

2. Проверьте логи:
```bash
# Из папки docker
docker-compose logs postgres

# Или из корня проекта
docker-compose -f docker/docker-compose.yml logs postgres
```

3. Убедитесь, что переменные окружения совпадают

### Очистка и перезапуск

```bash
# Из папки docker
# Остановить и удалить контейнеры
docker-compose down

# Удалить volumes (данные БД будут потеряны!)
docker-compose down -v

# Удалить образы
docker-compose down --rmi all

# Или из корня проекта
docker-compose -f docker/docker-compose.yml down

# Полная очистка
docker system prune -a
```

## Production настройки

Для production рекомендуется:

1. Изменить пароли в `docker/docker-compose.yml`
2. Использовать секреты Docker или переменные окружения
3. Настроить SSL/TLS
4. Использовать reverse proxy (nginx)
5. Настроить резервное копирование БД
6. Использовать health checks
7. Настроить мониторинг

Пример production `docker-compose.yml`:
```yaml
version: '3.8'

services:
  postgres:
    image: postgres:15-alpine
    environment:
      POSTGRES_DB: ${DB_NAME}
      POSTGRES_USER: ${DB_USER}
      POSTGRES_PASSWORD: ${DB_PASSWORD}
    volumes:
      - postgres_data:/var/lib/postgresql/data
    networks:
      - internal
    restart: always

  web:
    build: .
    environment:
      DB_HOST: postgres
      DB_PORT: 5432
      DB_NAME: ${DB_NAME}
      DB_USER: ${DB_USER}
      DB_PASSWORD: ${DB_PASSWORD}
    depends_on:
      - postgres
    networks:
      - internal
    restart: always

networks:
  internal:
    driver: bridge

volumes:
  postgres_data:
```

Создайте файл `.env`:
```
DB_NAME=infosec_db
DB_USER=postgres
DB_PASSWORD=your_secure_password_here
```

