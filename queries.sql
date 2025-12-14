-- queries.sql
-- Все SQL команды хранятся здесь для защиты от SQL-инъекций

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

-- Получение всех интеграторов
-- QUERY: GET_ALL_INTEGRATORS
SELECT id, name, city, description FROM integrators ORDER BY name;

-- Добавление интегратора (используется с параметрами)
-- QUERY: ADD_INTEGRATOR
INSERT INTO integrators (name, city, description) VALUES ($1, $2, $3) RETURNING id;

-- Обновление интегратора
-- QUERY: UPDATE_INTEGRATOR
UPDATE integrators SET name = $1, city = $2, description = $3 WHERE id = $4;

-- Удаление интегратора
-- QUERY: DELETE_INTEGRATOR
DELETE FROM integrators WHERE id = $1;

-- Поиск пользователя по имени
-- QUERY: GET_USER
SELECT id, username, password_hash, is_admin FROM users WHERE username = $1;

-- Создание пользователя
-- QUERY: CREATE_USER
INSERT INTO users (username, password_hash, is_admin) VALUES ($1, $2, $3) RETURNING id;

-- Создание сессии
-- QUERY: CREATE_SESSION
INSERT INTO sessions (session_id, user_id, expires_at) VALUES ($1, $2, $3) RETURNING id;

-- Получение сессии
-- QUERY: GET_SESSION
SELECT s.session_id, s.user_id, u.username, u.is_admin 
FROM sessions s 
JOIN users u ON s.user_id = u.id 
WHERE s.session_id = $1 AND s.expires_at > NOW();

-- Удаление сессии
-- QUERY: DELETE_SESSION
DELETE FROM sessions WHERE session_id = $1;

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