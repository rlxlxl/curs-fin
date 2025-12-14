#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <libpq-fe.h>

struct Integrator {
    int id;
    std::string name;
    std::string city;
    std::string description;
};

struct User {
    int id;
    std::string username;
    std::string passwordHash;
    bool isAdmin;
};

struct Session {
    std::string sessionId;
    int userId;
    std::string username;
    bool isAdmin;
};

class Database {
private:
    PGconn* conn;
    std::string connectionString;

public:
    Database(const std::string& host, const std::string& port, 
             const std::string& dbname, const std::string& user, 
             const std::string& password);
    ~Database();
    
    bool connect();
    void disconnect();
    
    // Методы для интеграторов
    std::vector<Integrator> getAllIntegrators();
    bool addIntegrator(const std::string& name, const std::string& city, 
                      const std::string& description);
    bool updateIntegrator(int id, const std::string& name, const std::string& city, 
                         const std::string& description);
    bool deleteIntegrator(int id);
    
    // Методы для пользователей
    User* getUserByUsername(const std::string& username);
    bool createUser(const std::string& username, const std::string& password, bool isAdmin = false);
    
    // Методы для сессий
    bool createSession(const std::string& sessionId, int userId);
    Session* getSession(const std::string& sessionId);
    bool deleteSession(const std::string& sessionId);
    bool deleteUserSessions(int userId);
};

#endif