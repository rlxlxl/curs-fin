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

-- Получение интеграторов по городу
-- QUERY: GET_INTEGRATORS_BY_CITY
SELECT id, name, city, description FROM integrators WHERE city = $1 ORDER BY name;

-- Поиск интеграторов по городу (частичное совпадение)
-- QUERY: SEARCH_INTEGRATORS_BY_CITY
SELECT id, name, city, description FROM integrators WHERE city ILIKE $1 ORDER BY name;

-- Получение списка всех уникальных городов
-- QUERY: GET_ALL_CITIES
SELECT DISTINCT city FROM integrators ORDER BY city;

-- Добавление интегратора (используется с параметрами)
-- QUERY: ADD_INTEGRATOR
INSERT INTO integrators (name, city, description) VALUES ($1, $2, $3);

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
INSERT INTO users (username, password_hash, is_admin) VALUES ($1, $2, $3);

-- Создание сессии
-- QUERY: CREATE_SESSION
INSERT INTO sessions (session_id, user_id, expires_at) VALUES ($1, $2, NOW() + INTERVAL '24 hours');

-- Получение сессии
-- QUERY: GET_SESSION
SELECT s.session_id, s.user_id, u.username, u.is_admin 
FROM sessions s 
JOIN users u ON s.user_id = u.id 
WHERE s.session_id = $1 AND s.expires_at > NOW();

-- Удаление сессии
-- QUERY: DELETE_SESSION
DELETE FROM sessions WHERE session_id = $1;

-- Удаление всех сессий пользователя
-- QUERY: DELETE_USER_SESSIONS
DELETE FROM sessions WHERE user_id = $1;

-- Добавление/обновление рейтинга (upsert)
-- QUERY: UPSERT_RATING
INSERT INTO ratings (integrator_id, user_id, rating, comment)
VALUES ($1, $2, $3, $4)
ON CONFLICT (integrator_id, user_id) DO UPDATE
SET rating = EXCLUDED.rating,
    comment = EXCLUDED.comment,
    created_at = CURRENT_TIMESTAMP;

-- Получение рейтингов по интегратору
-- QUERY: GET_RATINGS_BY_INTEGRATOR
SELECT r.id, r.integrator_id, r.user_id, r.rating, r.comment, r.created_at, u.username
FROM ratings r
JOIN users u ON r.user_id = u.id
WHERE r.integrator_id = $1
ORDER BY r.created_at DESC;

-- Получение агрегированной статистики рейтингов по всем интеграторам
-- QUERY: GET_RATING_STATS
SELECT integrator_id, AVG(rating) AS avg_rating, COUNT(*) AS rating_count
FROM ratings
GROUP BY integrator_id;

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