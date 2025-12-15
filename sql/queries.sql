-- queries.sql
-- Все SQL команды хранятся здесь для защиты от SQL-инъекций

-- Создание таблицы стран
CREATE TABLE IF NOT EXISTS countries (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL
);

-- Создание таблицы интеграторов
CREATE TABLE IF NOT EXISTS integrators (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    city VARCHAR(100) NOT NULL,
    description TEXT,
    website VARCHAR(255),
    country_id INTEGER REFERENCES countries(id)
);

-- Создание таблицы лицензий
CREATE TABLE IF NOT EXISTS licenses (
    id SERIAL PRIMARY KEY,
    integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE,
    license_number VARCHAR(100) NOT NULL,
    issued_by VARCHAR(255) NOT NULL
);

-- Создание таблицы сертификатов
CREATE TABLE IF NOT EXISTS certificates (
    id SERIAL PRIMARY KEY,
    integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE,
    certificate_name VARCHAR(255) NOT NULL,
    certificate_number VARCHAR(100),
    issued_by VARCHAR(255) NOT NULL
);

-- Создание таблицы продуктов
CREATE TABLE IF NOT EXISTS products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) UNIQUE NOT NULL
);

-- Создание таблицы услуг
CREATE TABLE IF NOT EXISTS services (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) UNIQUE NOT NULL
);

-- Создание таблицы связи интеграторов с продуктами (many-to-many)
CREATE TABLE IF NOT EXISTS integrator_products (
    integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE,
    product_id INTEGER REFERENCES products(id) ON DELETE CASCADE,
    PRIMARY KEY (integrator_id, product_id)
);

-- Создание таблицы связи интеграторов с услугами (many-to-many)
CREATE TABLE IF NOT EXISTS integrator_services (
    integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE,
    service_id INTEGER REFERENCES services(id) ON DELETE CASCADE,
    PRIMARY KEY (integrator_id, service_id)
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

-- Получение всех интеграторов с дополнительной информацией
-- QUERY: GET_ALL_INTEGRATORS
SELECT i.id, i.name, i.city, i.description, i.website, 
       c.name as country_name,
       COALESCE(
           (SELECT string_agg(p.name, ', ')
            FROM integrator_products ip
            JOIN products p ON ip.product_id = p.id
            WHERE ip.integrator_id = i.id),
           ''
       ) as products,
       COALESCE(
           (SELECT string_agg(s.name, ', ')
            FROM integrator_services iserv
            JOIN services s ON iserv.service_id = s.id
            WHERE iserv.integrator_id = i.id),
           ''
       ) as services
FROM integrators i
LEFT JOIN countries c ON i.country_id = c.id
ORDER BY i.name;

-- Получение интеграторов по городу
-- QUERY: GET_INTEGRATORS_BY_CITY
SELECT i.id, i.name, i.city, i.description, i.website, 
       c.name as country_name,
       COALESCE(
           (SELECT string_agg(p.name, ', ')
            FROM integrator_products ip
            JOIN products p ON ip.product_id = p.id
            WHERE ip.integrator_id = i.id),
           ''
       ) as products,
       COALESCE(
           (SELECT string_agg(s.name, ', ')
            FROM integrator_services iserv
            JOIN services s ON iserv.service_id = s.id
            WHERE iserv.integrator_id = i.id),
           ''
       ) as services
FROM integrators i
LEFT JOIN countries c ON i.country_id = c.id
WHERE i.city = $1 ORDER BY i.name;

-- Поиск интеграторов по городу (частичное совпадение)
-- QUERY: SEARCH_INTEGRATORS_BY_CITY
SELECT i.id, i.name, i.city, i.description, i.website, 
       c.name as country_name,
       COALESCE(
           (SELECT string_agg(p.name, ', ')
            FROM integrator_products ip
            JOIN products p ON ip.product_id = p.id
            WHERE ip.integrator_id = i.id),
           ''
       ) as products,
       COALESCE(
           (SELECT string_agg(s.name, ', ')
            FROM integrator_services iserv
            JOIN services s ON iserv.service_id = s.id
            WHERE iserv.integrator_id = i.id),
           ''
       ) as services
FROM integrators i
LEFT JOIN countries c ON i.country_id = c.id
WHERE i.city ILIKE $1 ORDER BY i.name;

-- Получение списка всех уникальных городов
-- QUERY: GET_ALL_CITIES
SELECT DISTINCT city FROM integrators ORDER BY city;

-- Добавление интегратора (используется с параметрами)
-- QUERY: ADD_INTEGRATOR
INSERT INTO integrators (name, city, description, website, country_id) 
VALUES ($1, $2, $3, 
    CASE WHEN $4 = '' OR $4 IS NULL THEN NULL ELSE $4 END,
    CASE WHEN $5 = '' OR $5 IS NULL OR $5::INTEGER = 0 THEN NULL ELSE $5::INTEGER END
) RETURNING id;

-- Обновление интегратора
-- QUERY: UPDATE_INTEGRATOR
UPDATE integrators SET 
    name = $1, 
    city = $2, 
    description = $3, 
    website = CASE WHEN $4 = '' OR $4 IS NULL THEN NULL ELSE $4 END,
    country_id = CASE WHEN $5 = '' OR $5 IS NULL OR $5::INTEGER = 0 THEN NULL ELSE $5::INTEGER END
WHERE id = $6;

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

-- Получение всех стран
-- QUERY: GET_ALL_COUNTRIES
SELECT id, name FROM countries ORDER BY name;

-- Получение всех продуктов
-- QUERY: GET_ALL_PRODUCTS
SELECT id, name FROM products ORDER BY name;

-- Получение всех услуг
-- QUERY: GET_ALL_SERVICES
SELECT id, name FROM services ORDER BY name;

-- Добавление лицензии
-- QUERY: ADD_LICENSE
INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES ($1, $2, $3);

-- Удаление лицензий интегратора
-- QUERY: DELETE_LICENSES
DELETE FROM licenses WHERE integrator_id = $1;

-- Добавление связи интегратора с продуктом
-- QUERY: ADD_INTEGRATOR_PRODUCT
INSERT INTO integrator_products (integrator_id, product_id) VALUES ($1, $2) ON CONFLICT DO NOTHING;

-- Удаление связей интегратора с продуктами
-- QUERY: DELETE_INTEGRATOR_PRODUCTS
DELETE FROM integrator_products WHERE integrator_id = $1;

-- Добавление связи интегратора с услугой
-- QUERY: ADD_INTEGRATOR_SERVICE
INSERT INTO integrator_services (integrator_id, service_id) VALUES ($1, $2) ON CONFLICT DO NOTHING;

-- Удаление связей интегратора с услугами
-- QUERY: DELETE_INTEGRATOR_SERVICES
DELETE FROM integrator_services WHERE integrator_id = $1;

-- Получение ID страны по названию
-- QUERY: GET_COUNTRY_ID
SELECT id FROM countries WHERE name = $1;

-- Получение ID продукта по названию
-- QUERY: GET_PRODUCT_ID
SELECT id FROM products WHERE name = $1;

-- Получение ID услуги по названию
-- QUERY: GET_SERVICE_ID
SELECT id FROM services WHERE name = $1;

-- Получение лицензий интегратора
-- QUERY: GET_LICENSES_BY_INTEGRATOR
SELECT license_number, issued_by FROM licenses WHERE integrator_id = $1 ORDER BY id;

-- Получение сертификатов интегратора
-- QUERY: GET_CERTIFICATES_BY_INTEGRATOR
SELECT certificate_name, certificate_number, issued_by FROM certificates WHERE integrator_id = $1 ORDER BY id;

-- Получение всех стран с ID
-- QUERY: GET_ALL_COUNTRIES_WITH_ID
SELECT id, name FROM countries ORDER BY name;

-- Получение всех продуктов с ID
-- QUERY: GET_ALL_PRODUCTS_WITH_ID
SELECT id, name FROM products ORDER BY name;

-- Получение всех услуг с ID
-- QUERY: GET_ALL_SERVICES_WITH_ID
SELECT id, name FROM services ORDER BY name;

-- Добавление лицензии
-- QUERY: ADD_LICENSE
INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES ($1, $2, $3);

-- Удаление всех лицензий интегратора
-- QUERY: DELETE_LICENSES
DELETE FROM licenses WHERE integrator_id = $1;

-- Добавление сертификата
-- QUERY: ADD_CERTIFICATE
INSERT INTO certificates (integrator_id, certificate_name, certificate_number, issued_by) VALUES ($1, $2, $3, $4);

-- Удаление всех сертификатов интегратора
-- QUERY: DELETE_CERTIFICATES
DELETE FROM certificates WHERE integrator_id = $1;

-- Удаление всех связей интегратора с продуктами
-- QUERY: DELETE_INTEGRATOR_PRODUCTS
DELETE FROM integrator_products WHERE integrator_id = $1;

-- Добавление связи интегратора с продуктом
-- QUERY: ADD_INTEGRATOR_PRODUCT
INSERT INTO integrator_products (integrator_id, product_id) VALUES ($1, $2) ON CONFLICT DO NOTHING;

-- Удаление всех связей интегратора с услугами
-- QUERY: DELETE_INTEGRATOR_SERVICES
DELETE FROM integrator_services WHERE integrator_id = $1;

-- Добавление связи интегратора с услугой
-- QUERY: ADD_INTEGRATOR_SERVICE
INSERT INTO integrator_services (integrator_id, service_id) VALUES ($1, $2) ON CONFLICT DO NOTHING;