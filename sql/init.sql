-- Инициализация базы данных для Docker
-- Этот файл выполняется автоматически при первом запуске контейнера PostgreSQL

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

-- Добавление стран
INSERT INTO countries (name) VALUES 
('Россия'), ('США'), ('Германия'), ('Израиль'), ('Великобритания')
ON CONFLICT (name) DO NOTHING;

-- Добавление продуктов
INSERT INTO products (name) VALUES 
('SIEM-системы'), ('Firewall'), ('Антивирусное ПО'), ('DLP-системы'), 
('PKI-решения'), ('VPN-решения'), ('IDS/IPS'), ('Шифрование данных'),
('Управление идентификацией'), ('Анализ угроз')
ON CONFLICT (name) DO NOTHING;

-- Добавление услуг
INSERT INTO services (name) VALUES 
('Аудит информационной безопасности'), ('Пентестинг'), ('Консалтинг по ИБ'),
('Внедрение систем защиты'), ('Обучение персонала'), ('Мониторинг безопасности'),
('Инцидент-менеджмент'), ('Управление рисками'), ('Сертификация по ISO 27001'),
('Разработка политик безопасности')
ON CONFLICT (name) DO NOTHING;

-- Реальные интеграторы с полной информацией
DO $$
DECLARE
    russia_id INTEGER;
    usa_id INTEGER;
    germany_id INTEGER;
    israel_id INTEGER;
    uk_id INTEGER;
    integrator1_id INTEGER;
    integrator2_id INTEGER;
    integrator3_id INTEGER;
    integrator4_id INTEGER;
    integrator5_id INTEGER;
    integrator6_id INTEGER;
    integrator7_id INTEGER;
    integrator8_id INTEGER;
    siem_id INTEGER;
    firewall_id INTEGER;
    antivirus_id INTEGER;
    dlp_id INTEGER;
    pki_id INTEGER;
    vpn_id INTEGER;
    ids_id INTEGER;
    encryption_id INTEGER;
    audit_id INTEGER;
    pentest_id INTEGER;
    consulting_id INTEGER;
    implementation_id INTEGER;
    training_id INTEGER;
    monitoring_id INTEGER;
BEGIN
    -- Получаем ID стран
    SELECT id INTO russia_id FROM countries WHERE name = 'Россия';
    SELECT id INTO usa_id FROM countries WHERE name = 'США';
    SELECT id INTO germany_id FROM countries WHERE name = 'Германия';
    SELECT id INTO israel_id FROM countries WHERE name = 'Израиль';
    SELECT id INTO uk_id FROM countries WHERE name = 'Великобритания';
    
    -- Получаем ID продуктов
    SELECT id INTO siem_id FROM products WHERE name = 'SIEM-системы';
    SELECT id INTO firewall_id FROM products WHERE name = 'Firewall';
    SELECT id INTO antivirus_id FROM products WHERE name = 'Антивирусное ПО';
    SELECT id INTO dlp_id FROM products WHERE name = 'DLP-системы';
    SELECT id INTO pki_id FROM products WHERE name = 'PKI-решения';
    SELECT id INTO vpn_id FROM products WHERE name = 'VPN-решения';
    SELECT id INTO ids_id FROM products WHERE name = 'IDS/IPS';
    SELECT id INTO encryption_id FROM products WHERE name = 'Шифрование данных';
    
    -- Получаем ID услуг
    SELECT id INTO audit_id FROM services WHERE name = 'Аудит информационной безопасности';
    SELECT id INTO pentest_id FROM services WHERE name = 'Пентестинг';
    SELECT id INTO consulting_id FROM services WHERE name = 'Консалтинг по ИБ';
    SELECT id INTO implementation_id FROM services WHERE name = 'Внедрение систем защиты';
    SELECT id INTO training_id FROM services WHERE name = 'Обучение персонала';
    SELECT id INTO monitoring_id FROM services WHERE name = 'Мониторинг безопасности';
    
    -- 1. Positive Technologies (Россия)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Positive Technologies', 'Москва', 'Ведущий российский разработчик решений в области информационной безопасности. Специализируется на тестировании на проникновение, анализе защищенности и управлении уязвимостями.', 'https://www.ptsecurity.com', russia_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator1_id;
    
    IF integrator1_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator1_id, 'ФСТЭК-001-2024', 'ФСТЭК России')
        ON CONFLICT DO NOTHING;
        INSERT INTO certificates (integrator_id, certificate_name, certificate_number, issued_by) VALUES
        (integrator1_id, 'ISO/IEC 27001:2022', 'ISO-27001-2024-PT', 'ISO Certification Body')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator1_id, siem_id), (integrator1_id, ids_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator1_id, audit_id), (integrator1_id, pentest_id), (integrator1_id, consulting_id)
        ON CONFLICT DO NOTHING;
    END IF;
    
    -- 2. InfoWatch (Россия)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('InfoWatch', 'Москва', 'Российская компания, специализирующаяся на защите конфиденциальной информации. Разработчик DLP-систем и решений для защиты от утечек данных.', 'https://www.infowatch.com', russia_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator2_id;
    
    IF integrator2_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator2_id, 'ФСТЭК-002-2024', 'ФСТЭК России')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator2_id, dlp_id), (integrator2_id, encryption_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator2_id, implementation_id), (integrator2_id, consulting_id)
        ON CONFLICT DO NOTHING;
    END IF;
    
    -- 3. Код Безопасности (Россия)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Код Безопасности', 'Москва', 'Ведущий российский интегратор в области информационной безопасности. Предоставляет комплексные решения для защиты корпоративной инфраструктуры.', 'https://www.securitycode.ru', russia_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator3_id;
    
    IF integrator3_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator3_id, 'ФСТЭК-003-2024', 'ФСТЭК России')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator3_id, firewall_id), (integrator3_id, vpn_id), (integrator3_id, siem_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator3_id, audit_id), (integrator3_id, implementation_id), (integrator3_id, training_id)
        ON CONFLICT DO NOTHING;
    END IF;
    
    -- 4. Palo Alto Networks (США)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Palo Alto Networks', 'Санта-Клара', 'Американская компания, мировой лидер в области сетевой безопасности. Разработчик новейших технологий защиты от киберугроз.', 'https://www.paloaltonetworks.com', usa_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator4_id;
    
    IF integrator4_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator4_id, 'NIST-2024-001', 'NIST (США)')
        ON CONFLICT DO NOTHING;
        INSERT INTO certificates (integrator_id, certificate_name, certificate_number, issued_by) VALUES
        (integrator4_id, 'ISO/IEC 27001:2022', 'ISO-27001-2024-PAN', 'ISO Certification Body'),
        (integrator4_id, 'SOC 2 Type II', 'SOC2-2024-PAN', 'AICPA')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator4_id, firewall_id), (integrator4_id, siem_id), (integrator4_id, ids_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator4_id, consulting_id), (integrator4_id, implementation_id), (integrator4_id, monitoring_id)
        ON CONFLICT DO NOTHING;
    END IF;
    
    -- 5. Check Point Software (Израиль)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Check Point Software', 'Тель-Авив', 'Израильская компания, один из мировых лидеров в области кибербезопасности. Специализируется на защите сетей, облачных сред и мобильных устройств.', 'https://www.checkpoint.com', israel_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator5_id;
    
    IF integrator5_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator5_id, 'ISO-27001-2024', 'ISO Certification Body')
        ON CONFLICT DO NOTHING;
        INSERT INTO certificates (integrator_id, certificate_name, certificate_number, issued_by) VALUES
        (integrator5_id, 'ISO/IEC 27001:2022', 'ISO-27001-2024-CP', 'ISO Certification Body'),
        (integrator5_id, 'Common Criteria EAL4+', 'CC-EAL4-2024-CP', 'Common Criteria')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator5_id, firewall_id), (integrator5_id, vpn_id), (integrator5_id, encryption_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator5_id, pentest_id), (integrator5_id, implementation_id), (integrator5_id, monitoring_id)
        ON CONFLICT DO NOTHING;
    END IF;
    
    -- 6. Kaspersky (Россия)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Kaspersky', 'Москва', 'Международная компания по кибербезопасности, основанная в России. Разработчик антивирусного ПО и решений для защиты от киберугроз.', 'https://www.kaspersky.com', russia_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator6_id;
    
    IF integrator6_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator6_id, 'ФСТЭК-004-2024', 'ФСТЭК России')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator6_id, antivirus_id), (integrator6_id, dlp_id), (integrator6_id, encryption_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator6_id, audit_id), (integrator6_id, consulting_id), (integrator6_id, training_id)
        ON CONFLICT DO NOTHING;
    END IF;
    
    -- 7. Fortinet (США)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Fortinet', 'Саннивейл', 'Американская компания, разработчик интегрированных решений безопасности. Предоставляет комплексную защиту сетей, приложений и облачных сред.', 'https://www.fortinet.com', usa_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator7_id;
    
    IF integrator7_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator7_id, 'NIST-2024-002', 'NIST (США)')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator7_id, firewall_id), (integrator7_id, siem_id), (integrator7_id, vpn_id), (integrator7_id, ids_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator7_id, implementation_id), (integrator7_id, consulting_id), (integrator7_id, monitoring_id)
ON CONFLICT DO NOTHING;
    END IF;
    
    -- 8. Sophos (Великобритания)
    INSERT INTO integrators (name, city, description, website, country_id) 
    VALUES ('Sophos', 'Оксфорд', 'Британская компания, специализирующаяся на кибербезопасности для бизнеса. Разработчик решений для защиты конечных точек, сетей и облачных сред.', 'https://www.sophos.com', uk_id)
    ON CONFLICT DO NOTHING
    RETURNING id INTO integrator8_id;
    
    IF integrator8_id IS NOT NULL THEN
        INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES
        (integrator8_id, 'ISO-27001-2024-UK', 'BSI (Великобритания)')
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_products (integrator_id, product_id) VALUES
        (integrator8_id, antivirus_id), (integrator8_id, firewall_id), (integrator8_id, encryption_id)
        ON CONFLICT DO NOTHING;
        INSERT INTO integrator_services (integrator_id, service_id) VALUES
        (integrator8_id, audit_id), (integrator8_id, implementation_id), (integrator8_id, training_id)
        ON CONFLICT DO NOTHING;
    END IF;
END $$;

