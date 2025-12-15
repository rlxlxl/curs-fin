-- Удаление всех таблиц из базы данных
-- ВНИМАНИЕ: Это удалит все данные!
-- Использует CASCADE для автоматического удаления зависимостей

-- Удаление всех таблиц в правильном порядке (с учетом зависимостей)
-- CASCADE автоматически удалит все зависимости (foreign keys, constraints)
DROP TABLE IF EXISTS sessions CASCADE;
DROP TABLE IF EXISTS ratings CASCADE;
DROP TABLE IF EXISTS integrator_services CASCADE;
DROP TABLE IF EXISTS integrator_products CASCADE;
DROP TABLE IF EXISTS certificates CASCADE;
DROP TABLE IF EXISTS licenses CASCADE;
DROP TABLE IF EXISTS integrators CASCADE;
DROP TABLE IF EXISTS services CASCADE;
DROP TABLE IF EXISTS products CASCADE;
DROP TABLE IF EXISTS countries CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- Альтернативный способ: удалить все таблицы через цикл
-- DO $$
-- DECLARE
--     r RECORD;
-- BEGIN
--     FOR r IN (SELECT tablename FROM pg_tables WHERE schemaname = 'public') LOOP
--         EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(r.tablename) || ' CASCADE';
--     END LOOP;
-- END $$;

