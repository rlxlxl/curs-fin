#include "database.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>

Database::Database(const std::string& host, const std::string& port, 
                   const std::string& dbname, const std::string& user, 
                   const std::string& password) {
    connectionString = "host=" + host + 
                      " port=" + port + 
                      " dbname=" + dbname + 
                      " user=" + user + 
                      " password=" + password;
    conn = nullptr;
    loadQueries("queries.sql");
}

Database::~Database() {
    disconnect();
}

bool Database::loadQueries(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка открытия файла queries.sql" << std::endl;
        return false;
    }
    
    std::string line;
    std::string currentQuery;
    std::string currentKey;
    bool inQuery = false;
    
    while (std::getline(file, line)) {
        // Убираем символ возврата каретки, если есть
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Проверяем, является ли строка комментарием с меткой QUERY
        size_t queryPos = line.find("-- QUERY:");
        if (queryPos != std::string::npos) {
            // Сохраняем предыдущий запрос, если он был
            if (inQuery && !currentKey.empty() && !currentQuery.empty()) {
                // Убираем лишние пробелы в начале и конце
                while (!currentQuery.empty() && currentQuery[0] == ' ') {
                    currentQuery.erase(0, 1);
                }
                while (!currentQuery.empty() && currentQuery.back() == ' ') {
                    currentQuery.pop_back();
                }
                if (!currentQuery.empty()) {
                    queries[currentKey] = currentQuery;
                }
            }
            
            // Извлекаем имя запроса
            size_t keyStart = queryPos + 9; // "-- QUERY:" = 9 символов
            while (keyStart < line.length() && line[keyStart] == ' ') {
                keyStart++;
            }
            size_t keyEnd = keyStart;
            while (keyEnd < line.length() && line[keyEnd] != ' ' && line[keyEnd] != '\r' && line[keyEnd] != '\n') {
                keyEnd++;
            }
            currentKey = line.substr(keyStart, keyEnd - keyStart);
            currentQuery.clear();
            inQuery = true;
        } else if (inQuery) {
            // Пропускаем пустые строки и комментарии
            bool isEmpty = true;
            for (char c : line) {
                if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                    isEmpty = false;
                    break;
                }
            }
            
            bool isComment = !line.empty() && line.length() >= 2 && line[0] == '-' && line[1] == '-';
            
            // Если строка не пустая и не комментарий - добавляем к запросу
            if (!isEmpty && !isComment) {
                // Добавляем строку к текущему запросу
                if (!currentQuery.empty()) {
                    currentQuery += " ";
                }
                // Убираем лишние пробелы из строки
                std::string trimmedLine = line;
                while (!trimmedLine.empty() && trimmedLine[0] == ' ') {
                    trimmedLine.erase(0, 1);
                }
                while (!trimmedLine.empty() && trimmedLine.back() == ' ') {
                    trimmedLine.pop_back();
                }
                currentQuery += trimmedLine;
                
                // Проверяем, заканчивается ли запрос точкой с запятой
                if (!trimmedLine.empty() && trimmedLine.back() == ';') {
                    // Запрос завершен - сохраняем его
                    while (!currentQuery.empty() && currentQuery[0] == ' ') {
                        currentQuery.erase(0, 1);
                    }
                    while (!currentQuery.empty() && currentQuery.back() == ' ') {
                        currentQuery.pop_back();
                    }
                    if (!currentKey.empty() && !currentQuery.empty()) {
                        queries[currentKey] = currentQuery;
                    }
                    currentQuery.clear();
                    currentKey.clear();
                    inQuery = false;
                }
            } else if ((isEmpty || isComment) && !currentQuery.empty()) {
                // Пустая строка или комментарий после запроса - сохраняем запрос
                while (!currentQuery.empty() && currentQuery[0] == ' ') {
                    currentQuery.erase(0, 1);
                }
                while (!currentQuery.empty() && currentQuery.back() == ' ') {
                    currentQuery.pop_back();
                }
                if (!currentKey.empty() && !currentQuery.empty()) {
                    queries[currentKey] = currentQuery;
                }
                currentQuery.clear();
                currentKey.clear();
                inQuery = false;
            }
        }
    }
    
    // Сохраняем последний запрос
    if (inQuery && !currentKey.empty() && !currentQuery.empty()) {
        while (!currentQuery.empty() && currentQuery[0] == ' ') {
            currentQuery.erase(0, 1);
        }
        while (!currentQuery.empty() && currentQuery.back() == ' ') {
            currentQuery.pop_back();
        }
        if (!currentQuery.empty()) {
            queries[currentKey] = currentQuery;
        }
    }
    
    file.close();
    std::cout << "Загружено SQL запросов: " << queries.size() << std::endl;
    return true;
}

bool Database::connect() {
    conn = PQconnectdb(connectionString.c_str());
    
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Ошибка подключения к БД: " << PQerrorMessage(conn) << std::endl;
        return false;
    }
    
    std::cout << "Подключение к БД успешно" << std::endl;
    return true;
}

void Database::disconnect() {
    if (conn) {
        PQfinish(conn);
        conn = nullptr;
    }
}

std::vector<Integrator> Database::getAllIntegrators() {
    std::vector<Integrator> integrators;
    
    if (queries.find("GET_ALL_INTEGRATORS") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_ALL_INTEGRATORS не найден" << std::endl;
        return integrators;
    }
    
    const char* query = queries["GET_ALL_INTEGRATORS"].c_str();
    
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return integrators;
    }
    
    int rows = PQntuples(res);
    
    for (int i = 0; i < rows; i++) {
        Integrator integrator;
        integrator.id = std::stoi(PQgetvalue(res, i, 0));
        integrator.name = PQgetvalue(res, i, 1);
        integrator.city = PQgetvalue(res, i, 2);
        integrator.description = PQgetvalue(res, i, 3);
        integrators.push_back(integrator);
    }
    
    PQclear(res);
    return integrators;
}

std::vector<Integrator> Database::getIntegratorsByCity(const std::string& city) {
    std::vector<Integrator> integrators;
    
    if (queries.find("GET_INTEGRATORS_BY_CITY") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_INTEGRATORS_BY_CITY не найден" << std::endl;
        return integrators;
    }
    
    const char* query = queries["GET_INTEGRATORS_BY_CITY"].c_str();
    
    const char* paramValues[1] = { city.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return integrators;
    }
    
    int rows = PQntuples(res);
    
    for (int i = 0; i < rows; i++) {
        Integrator integrator;
        integrator.id = std::stoi(PQgetvalue(res, i, 0));
        integrator.name = PQgetvalue(res, i, 1);
        integrator.city = PQgetvalue(res, i, 2);
        integrator.description = PQgetvalue(res, i, 3);
        integrators.push_back(integrator);
    }
    
    PQclear(res);
    return integrators;
}

std::vector<Integrator> Database::searchIntegratorsByCity(const std::string& cityPattern) {
    std::vector<Integrator> integrators;
    
    if (queries.find("SEARCH_INTEGRATORS_BY_CITY") == queries.end()) {
        std::cerr << "Ошибка: запрос SEARCH_INTEGRATORS_BY_CITY не найден" << std::endl;
        return integrators;
    }
    
    const char* query = queries["SEARCH_INTEGRATORS_BY_CITY"].c_str();
    
    // Формируем паттерн для поиска (добавляем % для частичного совпадения)
    std::string pattern = "%" + cityPattern + "%";
    const char* paramValues[1] = { pattern.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return integrators;
    }
    
    int rows = PQntuples(res);
    
    for (int i = 0; i < rows; i++) {
        Integrator integrator;
        integrator.id = std::stoi(PQgetvalue(res, i, 0));
        integrator.name = PQgetvalue(res, i, 1);
        integrator.city = PQgetvalue(res, i, 2);
        integrator.description = PQgetvalue(res, i, 3);
        integrators.push_back(integrator);
    }
    
    PQclear(res);
    return integrators;
}

std::vector<std::string> Database::getAllCities() {
    std::vector<std::string> cities;
    
    if (queries.find("GET_ALL_CITIES") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_ALL_CITIES не найден" << std::endl;
        return cities;
    }
    
    const char* query = queries["GET_ALL_CITIES"].c_str();
    
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return cities;
    }
    
    int rows = PQntuples(res);
    
    for (int i = 0; i < rows; i++) {
        cities.push_back(PQgetvalue(res, i, 0));
    }
    
    PQclear(res);
    return cities;
}

bool Database::addIntegrator(const std::string& name, const std::string& city, 
                             const std::string& description) {
    if (queries.find("ADD_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_INTEGRATOR не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["ADD_INTEGRATOR"].c_str();
    
    const char* paramValues[3] = {
        name.c_str(),
        city.c_str(),
        description.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 3, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка добавления: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::updateIntegrator(int id, const std::string& name, const std::string& city, 
                               const std::string& description) {
    if (queries.find("UPDATE_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос UPDATE_INTEGRATOR не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["UPDATE_INTEGRATOR"].c_str();
    
    std::string idStr = std::to_string(id);
    const char* paramValues[4] = {
        name.c_str(),
        city.c_str(),
        description.c_str(),
        idStr.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 4, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка обновления: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::deleteIntegrator(int id) {
    if (queries.find("DELETE_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_INTEGRATOR не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["DELETE_INTEGRATOR"].c_str();
    
    std::string idStr = std::to_string(id);
    const char* paramValues[1] = { idStr.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка удаления: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

User* Database::getUserByUsername(const std::string& username) {
    if (queries.find("GET_USER") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_USER не найден" << std::endl;
        return nullptr;
    }
    
    const char* query = queries["GET_USER"].c_str();
    
    const char* paramValues[1] = { username.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return nullptr;
    }
    
    User* user = new User();
    user->id = std::stoi(PQgetvalue(res, 0, 0));
    user->username = PQgetvalue(res, 0, 1);
    user->passwordHash = PQgetvalue(res, 0, 2);
    user->isAdmin = (strcmp(PQgetvalue(res, 0, 3), "t") == 0);
    
    PQclear(res);
    return user;
}

bool Database::createUser(const std::string& username, const std::string& password, bool isAdmin) {
    if (queries.find("CREATE_USER") == queries.end()) {
        std::cerr << "Ошибка: запрос CREATE_USER не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["CREATE_USER"].c_str();
    
    std::string isAdminStr = isAdmin ? "true" : "false";
    const char* paramValues[3] = {
        username.c_str(),
        password.c_str(),
        isAdminStr.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 3, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка создания пользователя: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::createSession(const std::string& sessionId, int userId) {
    if (queries.find("CREATE_SESSION") == queries.end()) {
        std::cerr << "Ошибка: запрос CREATE_SESSION не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["CREATE_SESSION"].c_str();
    
    std::string userIdStr = std::to_string(userId);
    const char* paramValues[2] = {
        sessionId.c_str(),
        userIdStr.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 2, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка создания сессии: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

Session* Database::getSession(const std::string& sessionId) {
    if (queries.find("GET_SESSION") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_SESSION не найден" << std::endl;
        return nullptr;
    }
    
    const char* query = queries["GET_SESSION"].c_str();
    
    const char* paramValues[1] = { sessionId.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return nullptr;
    }
    
    Session* session = new Session();
    session->sessionId = PQgetvalue(res, 0, 0);
    session->userId = std::stoi(PQgetvalue(res, 0, 1));
    session->username = PQgetvalue(res, 0, 2);
    session->isAdmin = (strcmp(PQgetvalue(res, 0, 3), "t") == 0);
    
    PQclear(res);
    return session;
}

bool Database::deleteSession(const std::string& sessionId) {
    if (queries.find("DELETE_SESSION") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_SESSION не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["DELETE_SESSION"].c_str();
    
    const char* paramValues[1] = { sessionId.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

bool Database::deleteUserSessions(int userId) {
    if (queries.find("DELETE_USER_SESSIONS") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_USER_SESSIONS не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["DELETE_USER_SESSIONS"].c_str();
    
    std::string userIdStr = std::to_string(userId);
    const char* paramValues[1] = { userIdStr.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка удаления сессий пользователя: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}