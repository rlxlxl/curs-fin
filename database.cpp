#include "database.h"
#include <iostream>
#include <cstring>

Database::Database(const std::string& host, const std::string& port, 
                   const std::string& dbname, const std::string& user, 
                   const std::string& password) {
    connectionString = "host=" + host + 
                      " port=" + port + 
                      " dbname=" + dbname + 
                      " user=" + user + 
                      " password=" + password;
    conn = nullptr;
}

Database::~Database() {
    disconnect();
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
    
    const char* query = "SELECT id, name, city, description FROM integrators ORDER BY name";
    
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

bool Database::addIntegrator(const std::string& name, const std::string& city, 
                             const std::string& description) {
    const char* query = "INSERT INTO integrators (name, city, description) VALUES ($1, $2, $3)";
    
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
    const char* query = "UPDATE integrators SET name = $1, city = $2, description = $3 WHERE id = $4";
    
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
    const char* query = "DELETE FROM integrators WHERE id = $1";
    
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
    const char* query = "SELECT id, username, password_hash, is_admin FROM users WHERE username = $1";
    
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
    const char* query = "INSERT INTO users (username, password_hash, is_admin) VALUES ($1, $2, $3)";
    
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
    const char* query = "INSERT INTO sessions (session_id, user_id, expires_at) VALUES ($1, $2, NOW() + INTERVAL '24 hours')";
    
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
    const char* query = "SELECT s.session_id, s.user_id, u.username, u.is_admin FROM sessions s JOIN users u ON s.user_id = u.id WHERE s.session_id = $1 AND s.expires_at > NOW()";
    
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
    const char* query = "DELETE FROM sessions WHERE session_id = $1";
    
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
    const char* query = "DELETE FROM sessions WHERE user_id = $1";
    
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