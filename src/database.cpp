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
    loadQueries("sql/queries.sql");
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
    
    // Проверяем и инициализируем данные по умолчанию, если их нет
    initializeDefaultData();
    
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
        integrator.website = PQgetvalue(res, i, 4) ? PQgetvalue(res, i, 4) : "";
        integrator.country = PQgetvalue(res, i, 5) ? PQgetvalue(res, i, 5) : "";
        integrator.products = PQgetvalue(res, i, 6) ? PQgetvalue(res, i, 6) : "";
        integrator.services = PQgetvalue(res, i, 7) ? PQgetvalue(res, i, 7) : "";
        
        // Загружаем лицензии и сертификаты отдельно
        integrator.licenses = getLicensesByIntegrator(integrator.id);
        integrator.certificates = getCertificatesByIntegrator(integrator.id);
        
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
        integrator.website = PQgetvalue(res, i, 4) ? PQgetvalue(res, i, 4) : "";
        integrator.country = PQgetvalue(res, i, 5) ? PQgetvalue(res, i, 5) : "";
        integrator.products = PQgetvalue(res, i, 6) ? PQgetvalue(res, i, 6) : "";
        integrator.services = PQgetvalue(res, i, 7) ? PQgetvalue(res, i, 7) : "";
        
        // Загружаем лицензии и сертификаты отдельно
        integrator.licenses = getLicensesByIntegrator(integrator.id);
        integrator.certificates = getCertificatesByIntegrator(integrator.id);
        
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
        integrator.website = PQgetvalue(res, i, 4) ? PQgetvalue(res, i, 4) : "";
        integrator.country = PQgetvalue(res, i, 5) ? PQgetvalue(res, i, 5) : "";
        integrator.products = PQgetvalue(res, i, 6) ? PQgetvalue(res, i, 6) : "";
        integrator.services = PQgetvalue(res, i, 7) ? PQgetvalue(res, i, 7) : "";
        
        // Загружаем лицензии и сертификаты отдельно
        integrator.licenses = getLicensesByIntegrator(integrator.id);
        integrator.certificates = getCertificatesByIntegrator(integrator.id);
        
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
    return addIntegrator(name, city, description, "", 0);
}

bool Database::addIntegrator(const std::string& name, const std::string& city, 
                             const std::string& description, const std::string& website, int countryId) {
    if (queries.find("ADD_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_INTEGRATOR не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["ADD_INTEGRATOR"].c_str();
    
    // Подготовка параметров
    std::string websiteParam = website.empty() ? "" : website;
    std::string countryIdParam = (countryId > 0) ? std::to_string(countryId) : "";
    
    const char* paramValues[5] = {
        name.c_str(),
        city.c_str(),
        description.c_str(),
        websiteParam.c_str(),
        countryIdParam.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 5, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка добавления: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    
    PQclear(res);
    return true;
}

int Database::addIntegratorAndGetId(const std::string& name, const std::string& city, 
                                    const std::string& description, const std::string& website, int countryId) {
    if (queries.find("ADD_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_INTEGRATOR не найден" << std::endl;
        return 0;
    }
    
    const char* query = queries["ADD_INTEGRATOR"].c_str();
    
    std::string websiteParam = website.empty() ? "" : website;
    std::string countryIdParam = (countryId > 0) ? std::to_string(countryId) : "";
    
    const char* paramValues[5] = {
        name.c_str(),
        city.c_str(),
        description.c_str(),
        websiteParam.c_str(),
        countryIdParam.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 5, nullptr, paramValues, 
                                 nullptr, nullptr, 0);
    
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        int id = std::stoi(PQgetvalue(res, 0, 0));
        PQclear(res);
        return id;
    } else if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        // Если запрос не вернул ID, получаем его отдельным запросом
        PQclear(res);
        std::string selectQuery = "SELECT id FROM integrators WHERE name = $1 AND city = $2 ORDER BY id DESC LIMIT 1";
        const char* selectParams[2] = { name.c_str(), city.c_str() };
        res = PQexecParams(conn, selectQuery.c_str(), 2, nullptr, selectParams, nullptr, nullptr, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
            int id = std::stoi(PQgetvalue(res, 0, 0));
            PQclear(res);
            return id;
        }
        PQclear(res);
        return 0;
    } else {
        std::cerr << "Ошибка добавления: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return 0;
    }
}

bool Database::updateIntegrator(int id, const std::string& name, const std::string& city, 
                               const std::string& description) {
    return updateIntegrator(id, name, city, description, "", 0);
}

bool Database::updateIntegrator(int id, const std::string& name, const std::string& city, 
                               const std::string& description, const std::string& website, int countryId) {
    if (queries.find("UPDATE_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос UPDATE_INTEGRATOR не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["UPDATE_INTEGRATOR"].c_str();
    
    // Подготовка параметров
    std::string websiteParam = website.empty() ? "" : website;
    std::string countryIdParam = (countryId > 0) ? std::to_string(countryId) : "";
    std::string idStr = std::to_string(id);
    
    const char* paramValues[6] = {
        name.c_str(),
        city.c_str(),
        description.c_str(),
        websiteParam.c_str(),
        countryIdParam.c_str(),
        idStr.c_str()
    };
    
    PGresult* res = PQexecParams(conn, query, 6, nullptr, paramValues, 
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

bool Database::addOrUpdateRating(int integratorId, int userId, int ratingValue, const std::string& comment) {
    if (queries.find("UPSERT_RATING") == queries.end()) {
        std::cerr << "Ошибка: запрос UPSERT_RATING не найден" << std::endl;
        return false;
    }

    const char* query = queries["UPSERT_RATING"].c_str();

    std::string integratorIdStr = std::to_string(integratorId);
    std::string userIdStr = std::to_string(userId);
    std::string ratingStr = std::to_string(ratingValue);

    const char* paramValues[4] = {
        integratorIdStr.c_str(),
        userIdStr.c_str(),
        ratingStr.c_str(),
        comment.c_str()
    };

    PGresult* res = PQexecParams(conn, query, 4, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка сохранения рейтинга: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

std::vector<Rating> Database::getRatingsByIntegrator(int integratorId) {
    std::vector<Rating> ratings;

    if (queries.find("GET_RATINGS_BY_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_RATINGS_BY_INTEGRATOR не найден" << std::endl;
        return ratings;
    }

    const char* query = queries["GET_RATINGS_BY_INTEGRATOR"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };

    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса рейтингов: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return ratings;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Rating r;
        r.id = std::stoi(PQgetvalue(res, i, 0));
        r.integratorId = std::stoi(PQgetvalue(res, i, 1));
        r.userId = std::stoi(PQgetvalue(res, i, 2));
        r.value = std::stoi(PQgetvalue(res, i, 3));
        r.comment = PQgetvalue(res, i, 4);
        r.createdAt = PQgetvalue(res, i, 5);
        r.username = PQgetvalue(res, i, 6);
        ratings.push_back(r);
    }

    PQclear(res);
    return ratings;
}

std::map<int, RatingStats> Database::getRatingStats() {
    std::map<int, RatingStats> stats;

    if (queries.find("GET_RATING_STATS") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_RATING_STATS не найден" << std::endl;
        return stats;
    }

    const char* query = queries["GET_RATING_STATS"].c_str();
    PGresult* res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса статистики рейтингов: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return stats;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        int integratorId = std::stoi(PQgetvalue(res, i, 0));
        double avg = std::stod(PQgetvalue(res, i, 1));
        int count = std::stoi(PQgetvalue(res, i, 2));
        stats[integratorId] = RatingStats{avg, count};
    }

    PQclear(res);
    return stats;
}

std::vector<License> Database::getLicensesByIntegrator(int integratorId) {
    std::vector<License> licenses;
    
    if (queries.find("GET_LICENSES_BY_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_LICENSES_BY_INTEGRATOR не найден" << std::endl;
        return licenses;
    }
    
    const char* query = queries["GET_LICENSES_BY_INTEGRATOR"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса лицензий: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return licenses;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        License license;
        license.number = PQgetvalue(res, i, 0);
        license.issuedBy = PQgetvalue(res, i, 1);
        licenses.push_back(license);
    }
    
    PQclear(res);
    return licenses;
}

std::vector<Certificate> Database::getCertificatesByIntegrator(int integratorId) {
    std::vector<Certificate> certificates;
    
    if (queries.find("GET_CERTIFICATES_BY_INTEGRATOR") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_CERTIFICATES_BY_INTEGRATOR не найден" << std::endl;
        return certificates;
    }
    
    const char* query = queries["GET_CERTIFICATES_BY_INTEGRATOR"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, nullptr, nullptr, 0);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса сертификатов: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return certificates;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        Certificate cert;
        cert.name = PQgetvalue(res, i, 0);
        cert.number = PQgetvalue(res, i, 1) ? PQgetvalue(res, i, 1) : "";
        cert.issuedBy = PQgetvalue(res, i, 2);
        certificates.push_back(cert);
    }
    
    PQclear(res);
    return certificates;
}

bool Database::addLicense(int integratorId, const std::string& licenseNumber, const std::string& issuedBy) {
    if (queries.find("ADD_LICENSE") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_LICENSE не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["ADD_LICENSE"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[3] = { integratorIdStr.c_str(), licenseNumber.c_str(), issuedBy.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 3, nullptr, paramValues, nullptr, nullptr, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        std::cerr << "Ошибка добавления лицензии: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(res);
    return success;
}

bool Database::deleteLicenses(int integratorId) {
    if (queries.find("DELETE_LICENSES") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_LICENSES не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["DELETE_LICENSES"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, nullptr, nullptr, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}

bool Database::addCertificate(int integratorId, const std::string& certificateName, const std::string& certificateNumber, const std::string& issuedBy) {
    if (queries.find("ADD_CERTIFICATE") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_CERTIFICATE не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["ADD_CERTIFICATE"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[4] = { integratorIdStr.c_str(), certificateName.c_str(), certificateNumber.c_str(), issuedBy.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 4, nullptr, paramValues, nullptr, nullptr, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    if (!success) {
        std::cerr << "Ошибка добавления сертификата: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(res);
    return success;
}

bool Database::deleteCertificates(int integratorId) {
    if (queries.find("DELETE_CERTIFICATES") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_CERTIFICATES не найден" << std::endl;
        return false;
    }
    
    const char* query = queries["DELETE_CERTIFICATES"].c_str();
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };
    
    PGresult* res = PQexecParams(conn, query, 1, nullptr, paramValues, nullptr, nullptr, 0);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}

std::vector<std::pair<int, std::string>> Database::getAllCountries() {
    std::vector<std::pair<int, std::string>> countries;
    
    if (queries.find("GET_ALL_COUNTRIES_WITH_ID") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_ALL_COUNTRIES_WITH_ID не найден" << std::endl;
        return countries;
    }
    
    const char* query = queries["GET_ALL_COUNTRIES_WITH_ID"].c_str();
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса стран: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return countries;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        countries.push_back({std::stoi(PQgetvalue(res, i, 0)), PQgetvalue(res, i, 1)});
    }
    
    PQclear(res);
    return countries;
}

std::vector<std::pair<int, std::string>> Database::getAllProducts() {
    std::vector<std::pair<int, std::string>> products;
    
    if (queries.find("GET_ALL_PRODUCTS_WITH_ID") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_ALL_PRODUCTS_WITH_ID не найден" << std::endl;
        return products;
    }
    
    const char* query = queries["GET_ALL_PRODUCTS_WITH_ID"].c_str();
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса продуктов: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return products;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        products.push_back({std::stoi(PQgetvalue(res, i, 0)), PQgetvalue(res, i, 1)});
    }
    
    PQclear(res);
    return products;
}

std::vector<std::pair<int, std::string>> Database::getAllServices() {
    std::vector<std::pair<int, std::string>> services;
    
    if (queries.find("GET_ALL_SERVICES_WITH_ID") == queries.end()) {
        std::cerr << "Ошибка: запрос GET_ALL_SERVICES_WITH_ID не найден" << std::endl;
        return services;
    }
    
    const char* query = queries["GET_ALL_SERVICES_WITH_ID"].c_str();
    PGresult* res = PQexec(conn, query);
    
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Ошибка запроса услуг: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return services;
    }
    
    int rows = PQntuples(res);
    for (int i = 0; i < rows; i++) {
        services.push_back({std::stoi(PQgetvalue(res, i, 0)), PQgetvalue(res, i, 1)});
    }
    
    PQclear(res);
    return services;
}

bool Database::setIntegratorProducts(int integratorId, const std::vector<int>& productIds) {
    // Удаляем старые связи
    if (queries.find("DELETE_INTEGRATOR_PRODUCTS") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_INTEGRATOR_PRODUCTS не найден" << std::endl;
        return false;
    }
    
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };
    PGresult* res = PQexecParams(conn, queries["DELETE_INTEGRATOR_PRODUCTS"].c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    PQclear(res);
    
    // Добавляем новые связи
    if (queries.find("ADD_INTEGRATOR_PRODUCT") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_INTEGRATOR_PRODUCT не найден" << std::endl;
        return false;
    }
    
    for (int productId : productIds) {
        std::string productIdStr = std::to_string(productId);
        const char* params[2] = { integratorIdStr.c_str(), productIdStr.c_str() };
        res = PQexecParams(conn, queries["ADD_INTEGRATOR_PRODUCT"].c_str(), 2, nullptr, params, nullptr, nullptr, 0);
        PQclear(res);
    }
    
    return true;
}

bool Database::setIntegratorServices(int integratorId, const std::vector<int>& serviceIds) {
    // Удаляем старые связи
    if (queries.find("DELETE_INTEGRATOR_SERVICES") == queries.end()) {
        std::cerr << "Ошибка: запрос DELETE_INTEGRATOR_SERVICES не найден" << std::endl;
        return false;
    }
    
    std::string integratorIdStr = std::to_string(integratorId);
    const char* paramValues[1] = { integratorIdStr.c_str() };
    PGresult* res = PQexecParams(conn, queries["DELETE_INTEGRATOR_SERVICES"].c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    PQclear(res);
    
    // Добавляем новые связи
    if (queries.find("ADD_INTEGRATOR_SERVICE") == queries.end()) {
        std::cerr << "Ошибка: запрос ADD_INTEGRATOR_SERVICE не найден" << std::endl;
        return false;
    }
    
    for (int serviceId : serviceIds) {
        std::string serviceIdStr = std::to_string(serviceId);
        const char* params[2] = { integratorIdStr.c_str(), serviceIdStr.c_str() };
        res = PQexecParams(conn, queries["ADD_INTEGRATOR_SERVICE"].c_str(), 2, nullptr, params, nullptr, nullptr, 0);
        PQclear(res);
    }
    
    return true;
}

bool Database::initializeDefaultData() {
    std::cout << "Проверка структуры БД..." << std::endl;
    
    // Всегда создаем таблицы, если их нет
    std::cout << "Создание таблиц, если их нет..." << std::endl;
    
    // Создаем таблицы, если их нет (из queries.sql)
    std::string createTables = 
        "CREATE TABLE IF NOT EXISTS countries (id SERIAL PRIMARY KEY, name VARCHAR(100) UNIQUE NOT NULL);"
        "CREATE TABLE IF NOT EXISTS integrators (id SERIAL PRIMARY KEY, name VARCHAR(255) NOT NULL, city VARCHAR(100) NOT NULL, description TEXT, website VARCHAR(255), country_id INTEGER REFERENCES countries(id));"
        "CREATE TABLE IF NOT EXISTS licenses (id SERIAL PRIMARY KEY, integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE, license_number VARCHAR(100) NOT NULL, issued_by VARCHAR(255) NOT NULL);"
        "CREATE TABLE IF NOT EXISTS certificates (id SERIAL PRIMARY KEY, integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE, certificate_name VARCHAR(255) NOT NULL, certificate_number VARCHAR(100), issued_by VARCHAR(255) NOT NULL);"
        "CREATE TABLE IF NOT EXISTS products (id SERIAL PRIMARY KEY, name VARCHAR(255) UNIQUE NOT NULL);"
        "CREATE TABLE IF NOT EXISTS services (id SERIAL PRIMARY KEY, name VARCHAR(255) UNIQUE NOT NULL);"
        "CREATE TABLE IF NOT EXISTS integrator_products (integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE, product_id INTEGER REFERENCES products(id) ON DELETE CASCADE, PRIMARY KEY (integrator_id, product_id));"
        "CREATE TABLE IF NOT EXISTS integrator_services (integrator_id INTEGER REFERENCES integrators(id) ON DELETE CASCADE, service_id INTEGER REFERENCES services(id) ON DELETE CASCADE, PRIMARY KEY (integrator_id, service_id));";
    
    PGresult* res = PQexec(conn, createTables.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка создания таблиц: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);
    std::cout << "Таблицы созданы/проверены." << std::endl;
    
    // Проверяем, есть ли уже интеграторы в БД
    PGresult* checkRes = PQexec(conn, "SELECT COUNT(*) FROM integrators");
    if (PQresultStatus(checkRes) == PGRES_TUPLES_OK) {
        int count = std::stoi(PQgetvalue(checkRes, 0, 0));
        PQclear(checkRes);
        if (count > 0) {
            std::cout << "В БД уже есть " << count << " интеграторов. Пропускаем заполнение данных." << std::endl;
            return true;
        }
    } else {
        PQclear(checkRes);
    }
    
    std::cout << "Инициализация данных по умолчанию..." << std::endl;
    
    // Добавляем страны
    std::string insertCountries = 
        "INSERT INTO countries (name) VALUES ('Россия'), ('США'), ('Германия'), ('Израиль'), ('Великобритания') "
        "ON CONFLICT (name) DO NOTHING;";
    res = PQexec(conn, insertCountries.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка добавления стран: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
    } else {
        PQclear(res);
    }
    
    // Добавляем продукты
    std::string insertProducts = 
        "INSERT INTO products (name) VALUES "
        "('SIEM-системы'), ('Firewall'), ('Антивирусное ПО'), ('DLP-системы'), "
        "('PKI-решения'), ('VPN-решения'), ('IDS/IPS'), ('Шифрование данных'), "
        "('Управление идентификацией'), ('Анализ угроз') "
        "ON CONFLICT (name) DO NOTHING;";
    res = PQexec(conn, insertProducts.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка добавления продуктов: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
    } else {
        PQclear(res);
    }
    
    // Добавляем услуги
    std::string insertServices = 
        "INSERT INTO services (name) VALUES "
        "('Аудит информационной безопасности'), ('Пентестинг'), ('Консалтинг по ИБ'), "
        "('Внедрение систем защиты'), ('Обучение персонала'), ('Мониторинг безопасности'), "
        "('Инцидент-менеджмент'), ('Управление рисками'), ('Сертификация по ISO 27001'), "
        "('Разработка политик безопасности') "
        "ON CONFLICT (name) DO NOTHING;";
    res = PQexec(conn, insertServices.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "Ошибка добавления услуг: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
    } else {
        PQclear(res);
    }
    
    // Получаем ID стран
    int russia_id = 0, usa_id = 0, israel_id = 0, uk_id = 0;
    res = PQexec(conn, "SELECT id FROM countries WHERE name = 'Россия'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        russia_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM countries WHERE name = 'США'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        usa_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM countries WHERE name = 'Израиль'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        israel_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM countries WHERE name = 'Великобритания'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        uk_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    // Получаем ID продуктов
    int siem_id = 0, firewall_id = 0, antivirus_id = 0, dlp_id = 0, vpn_id = 0, ids_id = 0, encryption_id = 0;
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'SIEM-системы'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        siem_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'Firewall'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        firewall_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'Антивирусное ПО'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        antivirus_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'DLP-системы'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        dlp_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'VPN-решения'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        vpn_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'IDS/IPS'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        ids_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM products WHERE name = 'Шифрование данных'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        encryption_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    // Получаем ID услуг
    int audit_id = 0, pentest_id = 0, consulting_id = 0, implementation_id = 0, training_id = 0, monitoring_id = 0;
    res = PQexec(conn, "SELECT id FROM services WHERE name = 'Аудит информационной безопасности'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        audit_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM services WHERE name = 'Пентестинг'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        pentest_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM services WHERE name = 'Консалтинг по ИБ'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        consulting_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM services WHERE name = 'Внедрение систем защиты'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        implementation_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM services WHERE name = 'Обучение персонала'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        training_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    res = PQexec(conn, "SELECT id FROM services WHERE name = 'Мониторинг безопасности'");
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        monitoring_id = std::stoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    
    // Добавляем интеграторов с полной информацией
    if (russia_id > 0) {
        // Positive Technologies
        if (addIntegrator("Positive Technologies", "Москва", 
            "Ведущий российский разработчик решений в области информационной безопасности. Специализируется на тестировании на проникновение, анализе защищенности и управлении уязвимостями.", 
            "https://www.ptsecurity.com", russia_id)) {
            // Получаем ID добавленного интегратора
            res = PQexec(conn, "SELECT id FROM integrators WHERE name = 'Positive Technologies'");
            if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
                int integrator_id = std::stoi(PQgetvalue(res, 0, 0));
                std::string licenseSQL = "INSERT INTO licenses (integrator_id, license_number, issued_by) VALUES (" + 
                    std::to_string(integrator_id) + ", 'ФСТЭК-001-2024', 'ФСТЭК России') ON CONFLICT DO NOTHING;";
                PQexec(conn, licenseSQL.c_str());
                if (siem_id > 0) {
                    std::string productSQL = "INSERT INTO integrator_products (integrator_id, product_id) VALUES (" + 
                        std::to_string(integrator_id) + ", " + std::to_string(siem_id) + ") ON CONFLICT DO NOTHING;";
                    PQexec(conn, productSQL.c_str());
                }
                if (ids_id > 0) {
                    std::string productSQL = "INSERT INTO integrator_products (integrator_id, product_id) VALUES (" + 
                        std::to_string(integrator_id) + ", " + std::to_string(ids_id) + ") ON CONFLICT DO NOTHING;";
                    PQexec(conn, productSQL.c_str());
                }
                if (audit_id > 0) {
                    std::string serviceSQL = "INSERT INTO integrator_services (integrator_id, service_id) VALUES (" + 
                        std::to_string(integrator_id) + ", " + std::to_string(audit_id) + ") ON CONFLICT DO NOTHING;";
                    PQexec(conn, serviceSQL.c_str());
                }
                if (pentest_id > 0) {
                    std::string serviceSQL = "INSERT INTO integrator_services (integrator_id, service_id) VALUES (" + 
                        std::to_string(integrator_id) + ", " + std::to_string(pentest_id) + ") ON CONFLICT DO NOTHING;";
                    PQexec(conn, serviceSQL.c_str());
                }
                if (consulting_id > 0) {
                    std::string serviceSQL = "INSERT INTO integrator_services (integrator_id, service_id) VALUES (" + 
                        std::to_string(integrator_id) + ", " + std::to_string(consulting_id) + ") ON CONFLICT DO NOTHING;";
                    PQexec(conn, serviceSQL.c_str());
                }
            }
            PQclear(res);
        }
        
        // Kaspersky
        addIntegrator("Kaspersky", "Москва", 
            "Международная компания по кибербезопасности, основанная в России. Разработчик антивирусного ПО и решений для защиты от киберугроз.", 
            "https://www.kaspersky.com", russia_id);
    }
    if (usa_id > 0) {
        addIntegrator("Palo Alto Networks", "Санта-Клара", 
            "Американская компания, мировой лидер в области сетевой безопасности. Разработчик новейших технологий защиты от киберугроз.", 
            "https://www.paloaltonetworks.com", usa_id);
        addIntegrator("Fortinet", "Саннивейл", 
            "Американская компания, разработчик интегрированных решений безопасности. Предоставляет комплексную защиту сетей, приложений и облачных сред.", 
            "https://www.fortinet.com", usa_id);
    }
    if (israel_id > 0) {
        addIntegrator("Check Point Software", "Тель-Авив", 
            "Израильская компания, один из мировых лидеров в области кибербезопасности. Специализируется на защите сетей, облачных сред и мобильных устройств.", 
            "https://www.checkpoint.com", israel_id);
    }
    if (uk_id > 0) {
        addIntegrator("Sophos", "Оксфорд", 
            "Британская компания, специализирующаяся на кибербезопасности для бизнеса. Разработчик решений для защиты конечных точек, сетей и облачных сред.", 
            "https://www.sophos.com", uk_id);
    }
    
    std::cout << "Инициализация данных завершена." << std::endl;
    return true;
}