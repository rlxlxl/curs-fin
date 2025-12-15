-- Полный сброс базы данных
-- Удаляет все таблицы и пересоздает их заново

-- Удаление всех таблиц
\i drop_all.sql

-- Создание таблиц заново
\i init.sql

-- Вывод информации о созданных таблицах
SELECT 
    table_name,
    (SELECT COUNT(*) FROM information_schema.columns WHERE table_name = t.table_name) as column_count
FROM information_schema.tables t
WHERE table_schema = 'public' 
  AND table_type = 'BASE TABLE'
ORDER BY table_name;

