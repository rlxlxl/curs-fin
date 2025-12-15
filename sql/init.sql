-- Инициализация базы данных для Docker
-- Этот файл выполняется автоматически при первом запуске контейнера PostgreSQL

-- Создание таблицы интеграторов
CREATE TABLE IF NOT EXISTS integrators (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    city VARCHAR(100) NOT NULL,
    description TEXT
);

-- Создание таблицы пользователей
CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    is_admin BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Создание таблицы сессий
CREATE TABLE IF NOT EXISTS sessions (
    id SERIAL PRIMARY KEY,
    session_id VARCHAR(255) UNIQUE NOT NULL,
    user_id INTEGER REFERENCES users(id),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL
);

-- Создание таблицы рейтингов и отзывов
CREATE TABLE IF NOT EXISTS ratings (
    id SERIAL PRIMARY KEY,
    integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    rating SMALLINT NOT NULL CHECK (rating BETWEEN 1 AND 5),
    comment TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (integrator_id, user_id)
);

-- Создание администратора по умолчанию (пароль: admin123)
INSERT INTO users (username, password_hash, is_admin) 
VALUES ('admin', 'admin123', TRUE) 
ON CONFLICT (username) DO NOTHING;

-- Примеры данных для тестирования
INSERT INTO integrators (name, city, description) VALUES 
('SecureIT Solutions', 'Москва', 'Комплексные решения по информационной безопасности для корпоративных клиентов'),
('CyberGuard Pro', 'Санкт-Петербург', 'Специализируется на защите критической инфраструктуры'),
('InfoDefense Group', 'Казань', 'Аудит безопасности и внедрение SIEM-систем')
ON CONFLICT DO NOTHING;

