#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <map>
#include <libpq-fe.h>

struct License {
    std::string number;
    std::string issuedBy;
};

struct Certificate {
    std::string name;
    std::string issuedBy;
    std::string number;
};

struct Integrator {
    int id;
    std::string name;
    std::string city;
    std::string description;
    std::string website;
    std::string country;
    std::vector<License> licenses;
    std::vector<Certificate> certificates;
    std::string products;
    std::string services;
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
    bool addIntegrator(const std::string& name, const std::string& city, 
                      const std::string& description, const std::string& website, int countryId);
    int addIntegratorAndGetId(const std::string& name, const std::string& city, 
                              const std::string& description, const std::string& website, int countryId);
    bool updateIntegrator(int id, const std::string& name, const std::string& city, 
                         const std::string& description);
    bool updateIntegrator(int id, const std::string& name, const std::string& city, 
                         const std::string& description, const std::string& website, int countryId);
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
    
    // Методы для лицензий и сертификатов
    std::vector<License> getLicensesByIntegrator(int integratorId);
    std::vector<Certificate> getCertificatesByIntegrator(int integratorId);
    bool addLicense(int integratorId, const std::string& licenseNumber, const std::string& issuedBy);
    bool deleteLicenses(int integratorId);
    bool addCertificate(int integratorId, const std::string& certificateName, const std::string& certificateNumber, const std::string& issuedBy);
    bool deleteCertificates(int integratorId);
    
    // Методы для получения справочников
    std::vector<std::pair<int, std::string>> getAllCountries();
    std::vector<std::pair<int, std::string>> getAllProducts();
    std::vector<std::pair<int, std::string>> getAllServices();
    
    // Методы для связывания интеграторов с продуктами и услугами
    bool setIntegratorProducts(int integratorId, const std::vector<int>& productIds);
    bool setIntegratorServices(int integratorId, const std::vector<int>& serviceIds);
    
    // Инициализация данных по умолчанию
    bool initializeDefaultData();
};

#endif