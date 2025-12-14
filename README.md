# Сайт интеграторов в сфере информационной безопасности

Простой веб-сервер на C++ с PostgreSQL для отображения информации об интеграторах.

## Структура проекта

```
project/
├── src/
│   ├── server.cpp      # Основной файл HTTP-сервера
│   └── database.cpp    # Реализация работы с БД
├── include/
│   └── database.h      # Заголовочный файл для работы с БД
├── sql/
│   └── queries.sql     # SQL запросы (защита от SQL-инъекций)
├── build/              # Скомпилированные файлы (создается автоматически)
├── Makefile            # Файл сборки
├── .gitignore          # Игнорируемые файлы для Git
└── README.md           # Эта инструкция
```

## Требования

- C++ компилятор (g++)
- PostgreSQL 12+
- libpq-dev (библиотека для работы с PostgreSQL)

## Установка зависимостей

### macOS:
```bash
# Установите Homebrew, если еще не установлен
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Установите PostgreSQL
brew install postgresql@15

# Установите компилятор (Xcode Command Line Tools)
xcode-select --install
```

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential postgresql postgresql-contrib libpq-dev
```

### Fedora/RHEL:
```bash
sudo dnf install gcc-c++ postgresql-server postgresql-devel
```

## Настройка базы данных

### macOS:

1. Запустите PostgreSQL:
```bash
# Запуск службы PostgreSQL
brew services start postgresql@15

# Или запуск вручную
pg_ctl -D /opt/homebrew/var/postgresql@15 start
```

2. Создайте базу данных:
```bash
# Подключитесь к PostgreSQL (на macOS обычно без sudo)
psql postgres
```

3. В PostgreSQL выполните:
```sql
CREATE DATABASE infosec_db;
CREATE USER postgres WITH PASSWORD 'password';
GRANT ALL PRIVILEGES ON DATABASE infosec_db TO postgres;
\c infosec_db
```

4. Создайте таблицу и добавьте тестовые данные:
```sql
CREATE TABLE IF NOT EXISTS integrators (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    city VARCHAR(100) NOT NULL,
    description TEXT
);

INSERT INTO integrators (name, city, description) VALUES 
('SecureIT Solutions', 'Москва', 'Комплексные решения по информационной безопасности для корпоративных клиентов'),
('CyberGuard Pro', 'Санкт-Петербург', 'Специализируется на защите критической инфраструктуры'),
('InfoDefense Group', 'Казань', 'Аудит безопасности и внедрение SIEM-систем');

\q
```

### Linux:

1. Запустите PostgreSQL:
```bash
sudo systemctl start postgresql
sudo systemctl enable postgresql
```

2. Создайте базу данных:
```bash
sudo -u postgres psql
```

3. В PostgreSQL выполните те же команды, что и для macOS (шаги 3-4 выше)

## Сборка и запуск

1. Скомпилируйте проект:
```bash
make
```

2. Запустите сервер:
```bash
make run
# или
./build/server
```

3. Откройте браузер и перейдите по адресу:
```
http://localhost:8080
```

## Защита от SQL-инъекций

Проект реализует несколько уровней защиты:

1. **SQL запросы в отдельном файле** (`queries.sql`) - не используется конкатенация строк
2. **Параметризованные запросы** - используется `PQexecParams()` с placeholder'ами
3. **Подготовленные выражения** - все параметры передаются отдельно от SQL

### Пример безопасного запроса:
```cpp
const char* query = "INSERT INTO integrators (name, city, description) VALUES ($1, $2, $3)";
const char* paramValues[3] = {name.c_str(), city.c_str(), description.c_str()};
PGresult* res = PQexecParams(conn, query, 3, nullptr, paramValues, nullptr, nullptr, 0);
```

## Настройка подключения к БД

Измените параметры в файле `src/server.cpp` (функция main):
```cpp
Database db("localhost", "5432", "infosec_db", "ваш_пользователь", "ваш_пароль");
```

## Очистка

Удалить скомпилированные файлы:
```bash
make clean
```

## Возможные ошибки

### macOS:

#### Ошибка компиляции - не найден libpq
```bash
# Найдите путь к PostgreSQL
brew --prefix postgresql@15

# Обновите Makefile, добавив пути:
CXXFLAGS = -std=c++17 -Wall -Wextra -I/opt/homebrew/opt/postgresql@15/include
LDFLAGS = -L/opt/homebrew/opt/postgresql@15/lib -lpq
```

#### PostgreSQL не запускается
```bash
# Проверьте статус
brew services list

# Переустановите, если нужно
brew reinstall postgresql@15

# Инициализируйте БД заново
initdb /opt/homebrew/var/postgresql@15
```

#### Ошибка подключения "role does not exist"
```bash
# Создайте пользователя с вашим системным именем
createuser -s $(whoami)
```

### Linux:

#### Ошибка подключения к БД
- Проверьте, что PostgreSQL запущен: `sudo systemctl status postgresql`
- Проверьте настройки в `pg_hba.conf` для локальных подключений

#### Ошибка компиляции
- Убедитесь, что установлен libpq-dev: `dpkg -l | grep libpq`
- Проверьте путь к заголовочным файлам PostgreSQL

### Общие проблемы:

#### Порт 8080 занят
- Измените порт в `server.cpp` на другой (например, 8081)

#### Кириллица отображается неправильно
- Убедитесь, что БД использует UTF-8: `SHOW SERVER_ENCODING;`
- При создании БД: `CREATE DATABASE infosec_db ENCODING 'UTF8';`

## Лицензия

MIT