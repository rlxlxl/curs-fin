-- Удаление всех таблиц из базы данных
-- ВНИМАНИЕ: Это удалит все данные!

-- Отключение проверки внешних ключей (для упрощения удаления)
SET session_replication_role = 'replica';

-- Удаление всех таблиц в правильном порядке (с учетом зависимостей)
DROP TABLE IF EXISTS sessions CASCADE;
DROP TABLE IF EXISTS ratings CASCADE;
DROP TABLE IF EXISTS integrators CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- Включение проверки внешних ключей обратно
SET session_replication_role = 'origin';

-- Альтернативный способ: удалить все таблицы через цикл
-- DO $$
-- DECLARE
--     r RECORD;
-- BEGIN
--     FOR r IN (SELECT tablename FROM pg_tables WHERE schemaname = 'public') LOOP
--         EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(r.tablename) || ' CASCADE';
--     END LOOP;
-- END $$;

