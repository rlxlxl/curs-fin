#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <map>
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

struct Rating {
    int id;
    int integratorId;
    int userId;
    int value;
    std::string comment;
    std::string username;
    std::string createdAt;
};

struct RatingStats {
    double average = 0.0;
    int count = 0;
};

class Database {
private:
    PGconn* conn;
    std::string connectionString;
    std::map<std::string, std::string> queries;
    
    bool loadQueries(const std::string& filename);

public:
    Database(const std::string& host, const std::string& port, 
             const std::string& dbname, const std::string& user, 
             const std::string& password);
    ~Database();
    
    bool connect();
    void disconnect();
    
    // Методы для интеграторов
    std::vector<Integrator> getAllIntegrators();
    std::vector<Integrator> getIntegratorsByCity(const std::string& city);
    std::vector<Integrator> searchIntegratorsByCity(const std::string& cityPattern);
    std::vector<std::string> getAllCities();
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

    // Методы для рейтингов и отзывов
    bool addOrUpdateRating(int integratorId, int userId, int ratingValue, const std::string& comment);
    std::vector<Rating> getRatingsByIntegrator(int integratorId);
    std::map<int, RatingStats> getRatingStats();
};

#endif