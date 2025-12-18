#include "database.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <random>
#include <algorithm>
#include <cctype>
#include <iomanip>

std::string generateSessionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    const char* hex = "0123456789abcdef";
    std::string sessionId;
    for (int i = 0; i < 32; i++) {
        sessionId += hex[dis(gen)];
    }
    return sessionId;
}

std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int value;
            std::istringstream(str.substr(i + 1, 2)) >> std::hex >> value;
            result += static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped << std::hex << std::uppercase;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << '+';
        } else {
            escaped << '%' << std::setw(2) << int(c) << std::setw(0);
        }
    }
    return escaped.str();
}

std::string htmlEscape(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '&') {
            result += "&amp;";
        } else if (c == '<') {
            result += "&lt;";
        } else if (c == '>') {
            result += "&gt;";
        } else if (c == '"') {
            result += "&quot;";
        } else if (c == '\'') {
            result += "&#39;";
        } else {
            result += c;
        }
    }
    return result;
}

std::string toLowerStr(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c){ return std::tolower(c); });
    return res;
}

bool containsCaseInsensitive(const std::string& text, const std::string& pattern) {
    if (pattern.empty()) return true;
    std::string lowerText = toLowerStr(text);
    std::string lowerPattern = toLowerStr(pattern);
    return lowerText.find(lowerPattern) != std::string::npos;
}

std::map<std::string, std::string> parsePostData(const std::string& data) {
    std::map<std::string, std::string> params;
    std::istringstream stream(data);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, pos));
            std::string value = urlDecode(pair.substr(pos + 1));
            params[key] = value;
        }
    }
    return params;
}

std::string getCookie(const std::string& headers, const std::string& name) {
    size_t pos = headers.find("Cookie:");
    if (pos == std::string::npos) return "";
    
    size_t start = headers.find(name + "=", pos);
    if (start == std::string::npos) return "";
    
    start += name.length() + 1;
    size_t end = headers.find_first_of(";\r\n", start);
    if (end == std::string::npos) end = headers.length();
    
    return headers.substr(start, end - start);
}

std::string getQueryParam(const std::string& request, const std::string& paramName) {
    size_t queryStart = request.find("?");
    if (queryStart == std::string::npos) return "";
    
    size_t queryEnd = request.find(" ", queryStart);
    if (queryEnd == std::string::npos) queryEnd = request.find("\r\n", queryStart);
    if (queryEnd == std::string::npos) return "";
    
    std::string queryString = request.substr(queryStart + 1, queryEnd - queryStart - 1);
    
    size_t paramPos = queryString.find(paramName + "=");
    if (paramPos == std::string::npos) return "";
    
    size_t valueStart = paramPos + paramName.length() + 1;
    size_t valueEnd = queryString.find("&", valueStart);
    if (valueEnd == std::string::npos) valueEnd = queryString.length();
    
    std::string value = queryString.substr(valueStart, valueEnd - valueStart);
    return urlDecode(value);
}

std::string generateLoginPage(const std::string& error = "") {
    std::ostringstream html;
    html << "<!DOCTYPE html><html lang='ru'><head>"
         << "<meta charset='UTF-8'><title>Вход в систему</title><style>"
         << "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }"
         << ".login-box { background: white; padding: 40px; border-radius: 10px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); width: 300px; }"
         << "h2 { text-align: center; color: #333; margin-bottom: 30px; }"
         << "input { width: 100%; padding: 12px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }"
         << "button { width: 100%; padding: 12px; background: #667eea; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin-top: 10px; }"
         << "button:hover { background: #5568d3; }"
         << ".error { color: red; text-align: center; margin-bottom: 10px; font-size: 14px; }"
         << ".info { color: #666; text-align: center; margin-top: 20px; font-size: 12px; }"
         << ".register-link { color: #667eea; text-decoration: none; display: block; text-align: center; margin-top: 15px; font-size: 14px; }"
         << ".register-link:hover { text-decoration: underline; }"
         << "</style>"
         << "<script>"
         << "window.onload = function() {"
         << "  sessionStorage.removeItem('authenticated');"
         << "};"
         << "</script>"
         << "</head><body><div class='login-box'><h2>🔐 Вход в систему</h2>";
    
    if (!error.empty()) {
        html << "<div class='error'>" << htmlEscape(error) << "</div>";
    }
    
    html << "<form method='POST' action='/login'>"
         << "<input type='text' name='username' placeholder='Имя пользователя' required>"
         << "<input type='password' name='password' placeholder='Пароль' required>"
         << "<button type='submit'>Войти</button></form>"
         << "<a href='/register' class='register-link'>Нет аккаунта? Зарегистрироваться</a>"
         << "</div></body></html>";
    
    return html.str();
}

std::string generateRegisterPage(const std::string& error = "", const std::string& username = "", const std::string& password = "") {
    std::ostringstream html;
    html << "<!DOCTYPE html><html lang='ru'><head>"
         << "<meta charset='UTF-8'><title>Регистрация</title><style>"
         << "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }"
         << ".register-box { background: white; padding: 40px; border-radius: 10px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); width: 320px; }"
         << "h2 { text-align: center; color: #333; margin-bottom: 30px; }"
         << "input { width: 100%; padding: 12px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }"
         << "button { width: 100%; padding: 12px; background: #27ae60; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin-top: 10px; }"
         << "button:hover { background: #229954; }"
         << ".error { color: red; text-align: center; margin-bottom: 10px; font-size: 14px; }"
         << ".info { color: #666; text-align: center; margin-top: 15px; font-size: 12px; }"
         << ".login-link { color: #667eea; text-decoration: none; display: block; text-align: center; margin-top: 15px; font-size: 14px; }"
         << ".login-link:hover { text-decoration: underline; }"
         << ".password-hint { font-size: 11px; color: #999; margin-top: -5px; margin-bottom: 10px; }"
         << "</style>"
         << "<script>"
         << "function validatePassword() {"
         << "  var pwd = document.getElementById('password').value;"
         << "  var confirmPwd = document.getElementById('password_confirm').value;"
         << "  var submitBtn = document.getElementById('submit-btn');"
         << "  if (pwd.length < 3) {"
         << "    submitBtn.disabled = true;"
         << "    return false;"
         << "  }"
         << "  if (pwd !== confirmPwd) {"
         << "    submitBtn.disabled = true;"
         << "    return false;"
         << "  }"
         << "  submitBtn.disabled = false;"
         << "  return true;"
         << "}"
         << "</script>"
         << "</head><body><div class='register-box'><h2>📝 Регистрация</h2>";
    
    if (!error.empty()) {
        html << "<div class='error'>" << htmlEscape(error) << "</div>";
    }
    
    std::string safeUsername = htmlEscape(username);
    std::string safePassword = htmlEscape(password);
    
    html << "<form method='POST' action='/register'>"
         << "<input type='text' name='username' id='username' placeholder='Имя пользователя' value='" << safeUsername << "' required minlength='3' maxlength='50'>"
         << "<input type='password' name='password' id='password' placeholder='Пароль (минимум 3 символа)' value='" << safePassword << "' required minlength='3' oninput='validatePassword()'>"
         << "<div class='password-hint'>Минимум 3 символа</div>"
         << "<input type='password' name='password_confirm' id='password_confirm' placeholder='Подтвердите пароль' required oninput='validatePassword()'>"
         << "<button type='submit' id='submit-btn'>Зарегистрироваться</button></form>"
         << "<a href='/login' class='login-link'>Уже есть аккаунт? Войти</a>"
         << "<div class='info'><br>После регистрации вы сможете оценивать интеграторов и оставлять отзывы</div>"
         << "</div></body></html>";
    
    return html.str();
}

std::string generateMainPage(
    const std::vector<Integrator>& integrators,
    bool isAdmin,
    bool isLoggedIn,
    const std::string& username,
    const std::string& tabToken,
    const std::vector<std::string>& cities,
    const std::vector<std::pair<int, std::string>>& countries = {},
    const std::vector<std::pair<int, std::string>>& products = {},
    const std::vector<std::pair<int, std::string>>& services = {},
    const std::string& cityQuery = "",
    const std::string& filterCityParam = "",
    const std::string& searchName = "",
    const std::string& sortOption = "name_asc",
    int page = 1,
    int totalPages = 1,
    int totalCount = 0,
    const std::map<int, RatingStats>& ratingStats = {},
    const std::map<int, std::vector<Rating>>& integratorRatings = {}
) {
    std::ostringstream html;
    html << "<!DOCTYPE html><html lang='ru'><head>"
         << "<meta charset='UTF-8'><title>Интеграторы InfoSec</title><style>"
         << "body { font-family: Arial, sans-serif; max-width: 1200px; margin: 0 auto; padding: 20px; background: #f5f5f5; }"
         << ".header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
         << "h1 { color: #2c3e50; margin: 0; }"
         << ".user-info { text-align: right; }"
         << ".user-name { color: #3498db; font-weight: bold; }"
         << ".admin-badge { background: #e74c3c; color: white; padding: 3px 8px; border-radius: 3px; font-size: 12px; margin-left: 10px; }"
         << ".logout-btn { background: #95a5a6; color: white; border: none; padding: 8px 16px; border-radius: 5px; cursor: pointer; margin-top: 10px; }"
         << ".integrator { background: white; padding: 20px; margin: 15px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); position: relative; }"
         << ".integrator h2 { color: #3498db; margin: 0 0 10px 0; }"
         << ".city { color: #7f8c8d; font-size: 14px; margin-bottom: 10px; }"
         << ".website, .licenses, .certificates, .products, .services { color: #7f8c8d; font-size: 14px; margin-bottom: 12px; }"
         << ".website a { color: #3498db; text-decoration: none; font-weight: 500; }"
         << ".website a:hover { text-decoration: underline; color: #2980b9; }"
         << ".license-list, .certificate-list { margin: 8px 0 0 20px; padding: 0; list-style: none; }"
         << ".license-list li, .certificate-list li { margin: 6px 0; padding: 8px; background: #f8f9fa; border-left: 3px solid #3498db; border-radius: 3px; }"
         << ".license-list li strong, .certificate-list li strong { color: #2c3e50; }"
         << ".license-list li em, .certificate-list li em { color: #7f8c8d; font-style: normal; }"
         << ".description { color: #34495e; line-height: 1.6; margin-top: 10px; }"
         << ".badge { display: inline-block; padding: 3px 8px; background: #3498db; color: white; border-radius: 3px; font-size: 12px; margin-right: 10px; }"
         << ".add-btn { background: #27ae60; color: white; border: none; padding: 12px 24px; border-radius: 5px; cursor: pointer; font-size: 16px; margin-bottom: 20px; }"
         << ".add-btn:hover { background: #229954; }"
         << ".action-buttons { position: absolute; top: 20px; right: 20px; }"
         << ".edit-btn, .delete-btn { padding: 6px 12px; margin-left: 5px; border: none; border-radius: 4px; cursor: pointer; font-size: 14px; }"
         << ".edit-btn { background: #f39c12; color: white; } .edit-btn:hover { background: #e67e22; }"
         << ".delete-btn { background: #e74c3c; color: white; } .delete-btn:hover { background: #c0392b; }"
         << ".modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); }"
         << ".modal-content { background: white; margin: 5% auto; padding: 30px; border-radius: 10px; width: 500px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }"
         << ".modal-content h2 { margin-top: 0; color: #2c3e50; }"
         << ".modal-content input, .modal-content textarea, .modal-content select { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }"
         << ".modal-content textarea { height: 100px; resize: vertical; }"
         << ".modal-content select[multiple] { height: 120px; }"
         << ".license-item, .certificate-item { display: flex; gap: 10px; margin-bottom: 10px; align-items: center; }"
         << ".license-item input, .certificate-item input { flex: 1; }"
         << ".add-item-btn { background: #3498db; color: white; border: none; padding: 8px 15px; border-radius: 5px; cursor: pointer; font-size: 14px; }"
         << ".add-item-btn:hover { background: #2980b9; }"
         << ".remove-item-btn { background: #e74c3c; color: white; border: none; padding: 8px 15px; border-radius: 5px; cursor: pointer; font-size: 14px; }"
         << ".remove-item-btn:hover { background: #c0392b; }"
         << ".items-container { margin: 10px 0; }"
         << ".modal-buttons { display: flex; justify-content: flex-end; gap: 10px; margin-top: 20px; }"
         << ".modal-buttons button { padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 14px; }"
         << ".save-btn { background: #27ae60; color: white; } .save-btn:hover { background: #229954; }"
         << ".cancel-btn { background: #95a5a6; color: white; } .cancel-btn:hover { background: #7f8c8d; }"
         << ".search-box { background: white; padding: 20px; margin-bottom: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
         << ".search-form { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }"
         << ".search-form input, .search-form select { padding: 10px; border: 1px solid #ddd; border-radius: 5px; font-size: 14px; }"
         << ".search-form input[type='text'] { flex: 1; min-width: 160px; }"
         << ".search-form select { min-width: 150px; }"
         << ".search-btn { background: #3498db; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-size: 14px; }"
         << ".search-btn:hover { background: #2980b9; }"
         << ".clear-btn { background: #95a5a6; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; font-size: 14px; }"
         << ".clear-btn:hover { background: #7f8c8d; }"
         << ".results-info { color: #7f8c8d; font-size: 14px; margin-bottom: 15px; }"
         << ".rating { margin-top: 8px; font-size: 14px; color: #555; }"
         << ".rating strong { color: #e67e22; }"
         << ".reviews { margin-top: 10px; background: #fafafa; padding: 10px; border: 1px solid #eee; border-radius: 6px; }"
         << ".review { margin-bottom: 8px; font-size: 13px; }"
         << ".pagination { margin-top: 15px; display: flex; gap: 8px; align-items: center; }"
         << ".pagination a, .pagination span { padding: 8px 12px; border-radius: 5px; border: 1px solid #ddd; text-decoration: none; color: #333; }"
         << ".pagination a:hover { background: #f0f0f0; }"
         << ".pagination .active { background: #3498db; color: white; border-color: #3498db; }"
         << ".rate-form { margin-top: 10px; display: flex; flex-direction: column; gap: 8px; }"
         << ".rate-form select, .rate-form textarea { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }"
         << ".rate-form button { align-self: flex-start; background: #3498db; color: white; border: none; padding: 8px 14px; border-radius: 5px; cursor: pointer; font-size: 14px; }"
         << ".rate-form button:hover { background: #2980b9; }"
         << "</style>"
         << "<script>"
         << "window.onload = function() {"
         << "  var storedToken = sessionStorage.getItem('tab_token');"
         << "  var serverToken = '" << tabToken << "';"
         << "  if (!storedToken) {"
         << "    sessionStorage.setItem('tab_token', serverToken);"
         << "  } else if (storedToken !== serverToken) {"
         << "    window.location.href = '/login_required';"
         << "    return;"
         << "  }"
         << "};"
         << "</script>"
         << "</head><body>"
         << "<div class='header'><h1>🛡️ Интеграторы InfoSec</h1>"
         << "<div class='user-info'><div class='user-name'>" << username;
    
    if (isAdmin) {
        html << "<span class='admin-badge'>ADMIN</span>";
    }
    
    html << "</div><form method='POST' action='/logout' style='display:inline;'>"
         << "<button type='submit' class='logout-btn'>Выйти</button></form></div></div>";
    
    // Форма поиска и фильтрации
    std::string escapedCity = htmlEscape(cityQuery);
    std::string escapedFilterCity = htmlEscape(filterCityParam);
    std::string escapedSearch = htmlEscape(searchName);
    html << "<div class='search-box'>"
         << "<form method='GET' action='/' class='search-form'>"
         << "<input type='text' name='name' placeholder='Поиск по названию...' value='" << escapedSearch << "'>"
         << "<input type='text' name='city' placeholder='Поиск по городу...' value='" << escapedCity << "'>"
         << "<select name='filter_city'>"
         << "<option value=''>Все города</option>";
    
    for (const auto& city : cities) {
        std::string escapedCityName = htmlEscape(city);
        html << "<option value='" << escapedCityName << "'";
        if (city == filterCityParam) {
            html << " selected";
        }
        html << ">" << escapedCityName << "</option>";
    }
    
    html << "</select>"
         << "<select name='sort'>"
         << "<option value='name_asc'" << (sortOption == "name_asc" ? " selected" : "") << ">Название ↑</option>"
         << "<option value='name_desc'" << (sortOption == "name_desc" ? " selected" : "") << ">Название ↓</option>"
         << "<option value='city_asc'" << (sortOption == "city_asc" ? " selected" : "") << ">Город ↑</option>"
         << "<option value='city_desc'" << (sortOption == "city_desc" ? " selected" : "") << ">Город ↓</option>"
         << "<option value='rating_desc'" << (sortOption == "rating_desc" ? " selected" : "") << ">Рейтинг ↓</option>"
         << "<option value='rating_asc'" << (sortOption == "rating_asc" ? " selected" : "") << ">Рейтинг ↑</option>"
         << "</select>"
         << "<button type='submit' class='search-btn'>🔍 Поиск</button>"
         << "<a href='/' style='text-decoration: none;'><button type='button' class='clear-btn'>Очистить</button></a>"
         << "</form>";
    
    int shownCount = static_cast<int>(integrators.size());
    int totalShown = totalCount > 0 ? totalCount : shownCount;
    html << "<div class='results-info'>Найдено интеграторов: " << totalShown << "</div>";
    
    html << "</div>";
    
    if (isAdmin) {
        html << "<button class='add-btn' onclick='openAddModal()'>➕ Добавить интегратора</button>";
    }
    
    for (const auto& integrator : integrators) {
        html << "<div class='integrator'>";
        
        if (isAdmin) {
            // Подготовка данных для модального окна
            std::string escapedName = integrator.name;
            std::string escapedCity = integrator.city;
            std::string escapedDesc = integrator.description;
            std::string escapedWebsite = integrator.website;
            
            // Экранирование кавычек и переносов строк
            auto escapeForJS = [](std::string& str) {
                size_t pos = 0;
                while ((pos = str.find("\\", pos)) != std::string::npos) {
                    str.replace(pos, 1, "\\\\");
                    pos += 2;
                }
                pos = 0;
                while ((pos = str.find("\"", pos)) != std::string::npos) {
                    str.replace(pos, 1, "\\\"");
                    pos += 2;
                }
                pos = 0;
                while ((pos = str.find("\n", pos)) != std::string::npos) {
                    str.replace(pos, 1, "\\n");
                    pos += 2;
                }
                pos = 0;
                while ((pos = str.find("\r", pos)) != std::string::npos) {
                    str.replace(pos, 1, "");
                }
            };
            
            escapeForJS(escapedName);
            escapeForJS(escapedCity);
            escapeForJS(escapedDesc);
            escapeForJS(escapedWebsite);
            
            // Получаем ID страны
            int countryId = 0;
            for (const auto& country : countries) {
                if (country.second == integrator.country) {
                    countryId = country.first;
                    break;
                }
            }
            
            // Получаем ID продуктов и услуг
            std::vector<int> productIds;
            std::vector<int> serviceIds;
            if (!integrator.products.empty()) {
                for (const auto& product : products) {
                    if (integrator.products.find(product.second) != std::string::npos) {
                        productIds.push_back(product.first);
                    }
                }
            }
            if (!integrator.services.empty()) {
                for (const auto& service : services) {
                    if (integrator.services.find(service.second) != std::string::npos) {
                        serviceIds.push_back(service.first);
                    }
                }
            }
            
            std::string productIdsStr;
            for (size_t i = 0; i < productIds.size(); i++) {
                if (i > 0) productIdsStr += ",";
                productIdsStr += std::to_string(productIds[i]);
            }
            
            std::string serviceIdsStr;
            for (size_t i = 0; i < serviceIds.size(); i++) {
                if (i > 0) serviceIdsStr += ",";
                serviceIdsStr += std::to_string(serviceIds[i]);
            }
            
            // Подготовка JSON для лицензий и сертификатов
            std::ostringstream licensesJson;
            licensesJson << "[";
            for (size_t i = 0; i < integrator.licenses.size(); i++) {
                if (i > 0) licensesJson << ",";
                std::string num = integrator.licenses[i].number;
                std::string issued = integrator.licenses[i].issuedBy;
                escapeForJS(num);
                escapeForJS(issued);
                licensesJson << "{\"number\":\"" << num << "\",\"issuedBy\":\"" << issued << "\"}";
            }
            licensesJson << "]";
            
            std::ostringstream certificatesJson;
            certificatesJson << "[";
            for (size_t i = 0; i < integrator.certificates.size(); i++) {
                if (i > 0) certificatesJson << ",";
                std::string name = integrator.certificates[i].name;
                std::string number = integrator.certificates[i].number;
                std::string issued = integrator.certificates[i].issuedBy;
                escapeForJS(name);
                escapeForJS(number);
                escapeForJS(issued);
                certificatesJson << "{\"name\":\"" << name << "\",\"number\":\"" << number << "\",\"issuedBy\":\"" << issued << "\"}";
            }
            certificatesJson << "]";
            
            html << "<div class='action-buttons'>"
                 << "<button class='edit-btn' onclick=\"openEditModal(" << integrator.id << ", '"
                 << escapedName << "', '" << escapedCity << "', '" << escapedDesc << "', '"
                 << escapedWebsite << "', " << countryId << ", '" << productIdsStr << "', '"
                 << serviceIdsStr << "', '" << licensesJson.str() << "', '" << certificatesJson.str() << "')\">✏️ Изменить</button>"
                 << "<form method='POST' action='/delete' style='display:inline;'>"
                 << "<input type='hidden' name='id' value='" << integrator.id << "'>"
                 << "<button type='submit' class='delete-btn' onclick='return confirm(\"Удалить этого интегратора?\")'>🗑️ Удалить</button>"
                 << "</form></div>";
        }
        
        html << "<h2>" << integrator.name << "</h2>"
             << "<div class='city'><span class='badge'>Город</span>" << integrator.city;
        if (!integrator.country.empty()) {
            html << " <span class='badge'>Страна</span>" << integrator.country;
        }
        html << "</div>";
        if (!integrator.website.empty()) {
            std::string websiteUrl = integrator.website;
            if (websiteUrl.find("http://") != 0 && websiteUrl.find("https://") != 0) {
                websiteUrl = "https://" + websiteUrl;
            }
            html << "<div class='website'><span class='badge'>🌐 Сайт</span><a href='" << htmlEscape(websiteUrl) << "' target='_blank' rel='noopener noreferrer'>" << htmlEscape(integrator.website) << " ↗</a></div>";
        }
        if (!integrator.licenses.empty()) {
            html << "<div class='licenses'><span class='badge'>📜 Лицензии</span><ul class='license-list'>";
            for (const auto& license : integrator.licenses) {
                html << "<li><strong>" << htmlEscape(license.number) << "</strong> — выдана: <em>" << htmlEscape(license.issuedBy) << "</em></li>";
            }
            html << "</ul></div>";
        }
        if (!integrator.certificates.empty()) {
            html << "<div class='certificates'><span class='badge'>🏆 Сертификаты</span><ul class='certificate-list'>";
            for (const auto& cert : integrator.certificates) {
                html << "<li><strong>" << htmlEscape(cert.name) << "</strong>";
                if (!cert.number.empty()) {
                    html << " (№ " << htmlEscape(cert.number) << ")";
                }
                html << " — выдано: <em>" << htmlEscape(cert.issuedBy) << "</em></li>";
            }
            html << "</ul></div>";
        }
        if (!integrator.products.empty()) {
            html << "<div class='products'><span class='badge'>Продукты</span>" << htmlEscape(integrator.products) << "</div>";
        }
        if (!integrator.services.empty()) {
            html << "<div class='services'><span class='badge'>Услуги</span>" << htmlEscape(integrator.services) << "</div>";
        }
        html << "<div class='description'>" << integrator.description << "</div>"
             << "<div class='rating'>";

        auto statIt = ratingStats.find(integrator.id);
        if (statIt != ratingStats.end() && statIt->second.count > 0) {
            html << "Рейтинг: <strong>" << std::fixed << std::setprecision(1) << statIt->second.average << "</strong> / 5"
                 << " (" << statIt->second.count << ")";
            html << std::defaultfloat;
        } else {
            html << "Рейтинг: нет оценок";
        }
        html << "</div>";

        auto ratingsIt = integratorRatings.find(integrator.id);
        if (ratingsIt != integratorRatings.end() && !ratingsIt->second.empty()) {
            html << "<div class='reviews'>";
            int shown = 0;
            for (const auto& r : ratingsIt->second) {
                if (shown >= 3) break;
                html << "<div class='review'>"
                     << "<strong>" << htmlEscape(r.username) << "</strong> — " << r.value << "/5"
                     << " <span style='color:#999;font-size:12px;'>" << r.createdAt << "</span><br>"
                     << htmlEscape(r.comment)
                     << "</div>";
                shown++;
            }
            html << "</div>";
        }

        if (isLoggedIn) {
            html << "<div class='rate-form'>"
                 << "<form method='POST' action='/rate'>"
                 << "<input type='hidden' name='id' value='" << integrator.id << "'>"
                 << "<label>Оцените интегратора:</label>"
                 << "<select name='rating'>"
                 << "<option value='5'>5</option>"
                 << "<option value='4'>4</option>"
                 << "<option value='3'>3</option>"
                 << "<option value='2'>2</option>"
                 << "<option value='1'>1</option>"
                 << "</select>"
                 << "<textarea name='comment' placeholder='Комментарий (необязательно)'></textarea>"
                 << "<button type='submit'>Сохранить оценку</button>"
                 << "</form>"
                 << "</div>";
        }

        html << "</div>";
    }

    // Пагинация
    if (totalPages > 1) {
        html << "<div class='pagination'>";
        auto makeLink = [&](int targetPage, const std::string& text, bool active) {
            std::ostringstream link;
            link << "/?page=" << targetPage
                 << "&name=" << urlEncode(searchName)
                 << "&city=" << urlEncode(cityQuery)
                 << "&filter_city=" << urlEncode(filterCityParam)
                 << "&sort=" << urlEncode(sortOption);
            if (active) {
                html << "<span class='active'>" << text << "</span>";
            } else {
                html << "<a href='" << link.str() << "'>" << text << "</a>";
            }
        };
        if (page > 1) {
            makeLink(page - 1, "« Назад", false);
        }
        makeLink(page, "Страница " + std::to_string(page) + " / " + std::to_string(totalPages), true);
        if (page < totalPages) {
            makeLink(page + 1, "Вперёд »", false);
        }
        html << "</div>";
    }
    
    if (isAdmin) {
        html << "<div id='modal' class='modal'><div class='modal-content' style='max-width: 700px; max-height: 90vh; overflow-y: auto;'>"
             << "<h2 id='modal-title'>Добавить интегратора</h2>"
             << "<form id='modal-form' method='POST' action='/add'>"
             << "<input type='hidden' name='id' id='edit-id'>"
             << "<input type='text' name='name' id='name' placeholder='Название' required>"
             << "<input type='text' name='city' id='city' placeholder='Город' required>"
             << "<textarea name='description' id='description' placeholder='Описание' required></textarea>"
             << "<input type='text' name='website' id='website' placeholder='Сайт (например: https://example.com)'>"
             << "<select name='country_id' id='country_id'>"
             << "<option value=''>Выберите страну</option>";
        
        for (const auto& country : countries) {
            html << "<option value='" << country.first << "'>" << htmlEscape(country.second) << "</option>";
        }
        
        html << "</select>"
             << "<label>Продукты (удерживайте Ctrl/Cmd для множественного выбора):</label>"
             << "<select name='products[]' id='products' multiple>";
        
        for (const auto& product : products) {
            html << "<option value='" << product.first << "'>" << htmlEscape(product.second) << "</option>";
        }
        
        html << "</select>"
             << "<label>Услуги (удерживайте Ctrl/Cmd для множественного выбора):</label>"
             << "<select name='services[]' id='services' multiple>";
        
        for (const auto& service : services) {
            html << "<option value='" << service.first << "'>" << htmlEscape(service.second) << "</option>";
        }
        
        html << "</select>"
             << "<label>Лицензии:</label>"
             << "<div id='licenses-container' class='items-container'></div>"
             << "<button type='button' class='add-item-btn' onclick='addLicenseField()'>+ Добавить лицензию</button>"
             << "<label>Сертификаты:</label>"
             << "<div id='certificates-container' class='items-container'></div>"
             << "<button type='button' class='add-item-btn' onclick='addCertificateField()'>+ Добавить сертификат</button>"
             << "<div class='modal-buttons'>"
             << "<button type='button' class='cancel-btn' onclick='closeModal()'>Отмена</button>"
             << "<button type='submit' class='save-btn'>Сохранить</button>"
             << "</div></form></div></div>"
             << "<script>"
             << "let licenseCount = 0;"
             << "let certificateCount = 0;"
             << "function addLicenseField() {"
             << "  const container = document.getElementById('licenses-container');"
             << "  const div = document.createElement('div');"
             << "  div.className = 'license-item';"
             << "  div.innerHTML = '<input type=\"text\" name=\"license_number[]\" placeholder=\"Номер лицензии\" required>"
             << "    <input type=\"text\" name=\"license_issued_by[]\" placeholder=\"Кем выдана\" required>"
             << "    <button type=\"button\" class=\"remove-item-btn\" onclick=\"this.parentElement.remove()\">Удалить</button>';"
             << "  container.appendChild(div);"
             << "  licenseCount++;"
             << "}"
             << "function addCertificateField() {"
             << "  const container = document.getElementById('certificates-container');"
             << "  const div = document.createElement('div');"
             << "  div.className = 'certificate-item';"
             << "  div.innerHTML = '<input type=\"text\" name=\"certificate_name[]\" placeholder=\"Название сертификата\" required>"
             << "    <input type=\"text\" name=\"certificate_number[]\" placeholder=\"Номер (необязательно)\">"
             << "    <input type=\"text\" name=\"certificate_issued_by[]\" placeholder=\"Кем выдан\" required>"
             << "    <button type=\"button\" class=\"remove-item-btn\" onclick=\"this.parentElement.remove()\">Удалить</button>';"
             << "  container.appendChild(div);"
             << "  certificateCount++;"
             << "}"
             << "function openAddModal() {"
             << "  document.getElementById('modal-title').innerText = 'Добавить интегратора';"
             << "  document.getElementById('modal-form').action = '/add';"
             << "  document.getElementById('edit-id').value = '';"
             << "  document.getElementById('name').value = '';"
             << "  document.getElementById('city').value = '';"
             << "  document.getElementById('description').value = '';"
             << "  document.getElementById('website').value = '';"
             << "  document.getElementById('country_id').value = '';"
             << "  Array.from(document.getElementById('products').options).forEach(opt => opt.selected = false);"
             << "  Array.from(document.getElementById('services').options).forEach(opt => opt.selected = false);"
             << "  document.getElementById('licenses-container').innerHTML = '';"
             << "  document.getElementById('certificates-container').innerHTML = '';"
             << "  licenseCount = 0;"
             << "  certificateCount = 0;"
             << "  document.getElementById('modal').style.display = 'block';"
             << "}"
             << "function openEditModal(id, name, city, desc, website, countryId, productIds, serviceIds, licenses, certificates) {"
             << "  document.getElementById('modal-title').innerText = 'Изменить интегратора';"
             << "  document.getElementById('modal-form').action = '/update';"
             << "  document.getElementById('edit-id').value = id;"
             << "  document.getElementById('name').value = name || '';"
             << "  document.getElementById('city').value = city || '';"
             << "  document.getElementById('description').value = desc || '';"
             << "  document.getElementById('website').value = website || '';"
             << "  document.getElementById('country_id').value = countryId || '';"
             << "  if (productIds) {"
             << "    const ids = productIds.split(',');"
             << "    Array.from(document.getElementById('products').options).forEach(opt => {"
             << "      opt.selected = ids.includes(opt.value);"
             << "    });"
             << "  }"
             << "  if (serviceIds) {"
             << "    const ids = serviceIds.split(',');"
             << "    Array.from(document.getElementById('services').options).forEach(opt => {"
             << "      opt.selected = ids.includes(opt.value);"
             << "    });"
             << "  }"
             << "  const licensesContainer = document.getElementById('licenses-container');"
             << "  licensesContainer.innerHTML = '';"
             << "  if (licenses) {"
             << "    const licenseList = JSON.parse(licenses);"
             << "    licenseList.forEach(function(lic) {"
             << "      const div = document.createElement('div');"
             << "      div.className = 'license-item';"
             << "      div.innerHTML = '<input type=\"text\" name=\"license_number[]\" value=\"' + (lic.number || '') + '\" placeholder=\"Номер лицензии\" required>"
             << "        <input type=\"text\" name=\"license_issued_by[]\" value=\"' + (lic.issuedBy || '') + '\" placeholder=\"Кем выдана\" required>"
             << "        <button type=\"button\" class=\"remove-item-btn\" onclick=\"this.parentElement.remove()\">Удалить</button>';"
             << "      licensesContainer.appendChild(div);"
             << "    });"
             << "  }"
             << "  const certificatesContainer = document.getElementById('certificates-container');"
             << "  certificatesContainer.innerHTML = '';"
             << "  if (certificates) {"
             << "    const certList = JSON.parse(certificates);"
             << "    certList.forEach(function(cert) {"
             << "      const div = document.createElement('div');"
             << "      div.className = 'certificate-item';"
             << "      div.innerHTML = '<input type=\"text\" name=\"certificate_name[]\" value=\"' + (cert.name || '') + '\" placeholder=\"Название сертификата\" required>"
             << "        <input type=\"text\" name=\"certificate_number[]\" value=\"' + (cert.number || '') + '\" placeholder=\"Номер (необязательно)\">"
             << "        <input type=\"text\" name=\"certificate_issued_by[]\" value=\"' + (cert.issuedBy || '') + '\" placeholder=\"Кем выдан\" required>"
             << "        <button type=\"button\" class=\"remove-item-btn\" onclick=\"this.parentElement.remove()\">Удалить</button>';"
             << "      certificatesContainer.appendChild(div);"
             << "    });"
             << "  }"
             << "  document.getElementById('modal').style.display = 'block';"
             << "}"
             << "function closeModal() { document.getElementById('modal').style.display = 'none'; }"
             << "window.onclick = function(event) { if (event.target == document.getElementById('modal')) { closeModal(); } }"
             << "</script>";
    }
    
    html << "</body></html>";
    return html.str();
}

std::string createHTTPResponse(const std::string& body, const std::string& setCookie = "") {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: text/html; charset=utf-8\r\n"
             << "Content-Length: " << body.length() << "\r\n";
    
    if (!setCookie.empty()) {
        response << "Set-Cookie: session_id=" << setCookie << "; Path=/; HttpOnly\r\n";
    }
    
    response << "Connection: close\r\n\r\n" << body;
    return response.str();
}

std::string createRedirectResponse(const std::string& location) {
    std::ostringstream response;
    response << "HTTP/1.1 302 Found\r\n"
             << "Location: " << location << "\r\n"
             << "Connection: close\r\n\r\n";
    return response.str();
}

std::string getEnv(const std::string& key, const std::string& defaultValue) {
    const char* val = std::getenv(key.c_str());
    return val ? std::string(val) : defaultValue;
}

int main() {
    // Получение параметров подключения из переменных окружения или использование значений по умолчанию
    std::string dbHost = getEnv("DB_HOST", "localhost");
    std::string dbPort = getEnv("DB_PORT", "5432");
    std::string dbName = getEnv("DB_NAME", "infosec_db");
    std::string dbUser = getEnv("DB_USER", "postgres");
    std::string dbPassword = getEnv("DB_PASSWORD", "password");
    
    Database db(dbHost, dbPort, dbName, dbUser, dbPassword);
    
    if (!db.connect()) {
        return 1;
    }
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        close(serverSocket);
        return 1;
    }
    
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Ошибка прослушивания" << std::endl;
        close(serverSocket);
        return 1;
    }
    
    std::cout << "Сервер запущен на http://localhost:8080" << std::endl;
    
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) continue;
        
        char buffer[8192] = {0};
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) {
            close(clientSocket);
            continue;
        }
        
        std::string request(buffer);
        std::string sessionId = getCookie(request, "session_id");
        std::cout << "Session ID из cookie: '" << sessionId << "'" << std::endl;
        Session* session = sessionId.empty() ? nullptr : db.getSession(sessionId);
        
        if (session) {
            std::cout << "Сессия найдена! User: " << session->username << ", Admin: " << session->isAdmin << std::endl;
        } else {
            std::cout << "Сессия не найдена" << std::endl;
        }
        
        std::string response;
        
        if (request.find("POST /login") == 0) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                std::string body = request.substr(bodyStart + 4);
                std::cout << "POST body: '" << body << "'" << std::endl;
                
                auto params = parsePostData(body);
                
                std::cout << "Параметры: ";
                for (const auto& p : params) {
                    std::cout << p.first << "='" << p.second << "' ";
                }
                std::cout << std::endl;
                
                std::string username = params["username"];
                std::string password = params["password"];
                
                std::cout << "Username: '" << username << "', Password: '" << password << "'" << std::endl;
                
                if (username.empty()) {
                    std::cout << "ОШИБКА: Имя пользователя пустое!" << std::endl;
                    response = createHTTPResponse(generateLoginPage("Ошибка: введите имя пользователя"));
                    send(clientSocket, response.c_str(), response.length(), 0);
                    close(clientSocket);
                    continue;
                }
                
                std::cout << "Попытка входа: " << username << std::endl;
                
                User* user = db.getUserByUsername(username);
                
                if (!user) {
                    std::cout << "Пользователь не найден" << std::endl;
                    response = createHTTPResponse(generateLoginPage("Пользователь не найден. Зарегистрируйтесь, пожалуйста."));
                    send(clientSocket, response.c_str(), response.length(), 0);
                    close(clientSocket);
                    continue;
                }
                
                std::cout << "Пользователь найден, isAdmin: " << user->isAdmin << std::endl;
                
                if (user->passwordHash == password) {
                    // Генерируем уникальный токен для этой вкладки
                    std::string tabToken = generateSessionId();
                    
                    std::string newSessionId = generateSessionId();
                    std::cout << "Пароль верный, создаём новую сессию: " << newSessionId << std::endl;
                    db.createSession(newSessionId, user->id);
                    
                    // Проверяем, что сессия создана
                    Session* checkSession = db.getSession(newSessionId);
                    if (checkSession) {
                        std::cout << "Сессия создана успешно! User: " << checkSession->username << ", Admin: " << checkSession->isAdmin << std::endl;
                        delete checkSession;
                    } else {
                        std::cout << "ОШИБКА: Сессия не создана!" << std::endl;
                    }
                    
                    // Создаём HTML страницу с редиректом и установкой sessionStorage + tab_token
                    std::string redirectPage = "<!DOCTYPE html><html><head><meta charset='UTF-8'><script>"
                        "sessionStorage.setItem('authenticated', 'true');"
                        "sessionStorage.setItem('tab_token', '" + tabToken + "');"
                        "window.location.href = '/?tab_token=" + tabToken + "';"
                        "</script></head><body>Перенаправление...</body></html>";
                    
                    std::ostringstream resp;
                    resp << "HTTP/1.1 200 OK\r\n"
                         << "Content-Type: text/html; charset=utf-8\r\n"
                         << "Set-Cookie: session_id=" << newSessionId << "; Path=/; HttpOnly\r\n"
                         << "Set-Cookie: tab_token=" << tabToken << "; Path=/\r\n"
                         << "Content-Length: " << redirectPage.length() << "\r\n"
                         << "Connection: close\r\n\r\n"
                         << redirectPage;
                    response = resp.str();
                } else {
                    std::cout << "Неверный пароль" << std::endl;
                    response = createHTTPResponse(generateLoginPage("Неверный пароль"));
                }
                
                delete user;
            }
        } else if (request.find("GET /register") == 0) {
            response = createHTTPResponse(generateRegisterPage());
        } else if (request.find("POST /register") == 0) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                std::string body = request.substr(bodyStart + 4);
                auto params = parsePostData(body);
                
                std::string username = params["username"];
                std::string password = params["password"];
                std::string passwordConfirm = params["password_confirm"];
                
                // Валидация
                if (username.empty() || username.length() < 3) {
                    response = createHTTPResponse(generateRegisterPage("Имя пользователя должно содержать минимум 3 символа", username, ""));
                } else if (password.empty() || password.length() < 3) {
                    response = createHTTPResponse(generateRegisterPage("Пароль должен содержать минимум 3 символа", username, ""));
                } else if (password != passwordConfirm) {
                    response = createHTTPResponse(generateRegisterPage("Пароли не совпадают", username, ""));
                } else {
                    // Проверка, существует ли пользователь
                    User* existingUser = db.getUserByUsername(username);
                    if (existingUser) {
                        delete existingUser;
                        response = createHTTPResponse(generateRegisterPage("Пользователь с таким именем уже существует", username, ""));
                    } else {
                        // Создание пользователя (admin только если имя "admin")
                        bool isAdmin = (username == "admin");
                        if (db.createUser(username, password, isAdmin)) {
                            // Автоматический вход после регистрации
                            User* newUser = db.getUserByUsername(username);
                            if (newUser) {
                                std::string tabToken = generateSessionId();
                                std::string newSessionId = generateSessionId();
                                db.createSession(newSessionId, newUser->id);
                                
                                std::string redirectPage = "<!DOCTYPE html><html><head><meta charset='UTF-8'><script>"
                                    "sessionStorage.setItem('authenticated', 'true');"
                                    "sessionStorage.setItem('tab_token', '" + tabToken + "');"
                                    "window.location.href = '/?tab_token=" + tabToken + "';"
                                    "</script></head><body>Регистрация успешна! Перенаправление...</body></html>";
                                
                                std::ostringstream resp;
                                resp << "HTTP/1.1 200 OK\r\n"
                                     << "Content-Type: text/html; charset=utf-8\r\n"
                                     << "Set-Cookie: session_id=" << newSessionId << "; Path=/; HttpOnly\r\n"
                                     << "Set-Cookie: tab_token=" << tabToken << "; Path=/\r\n"
                                     << "Content-Length: " << redirectPage.length() << "\r\n"
                                     << "Connection: close\r\n\r\n"
                                     << redirectPage;
                                response = resp.str();
                                
                                delete newUser;
                            } else {
                                response = createHTTPResponse(generateRegisterPage("Ошибка при создании пользователя", username, ""));
                            }
                        } else {
                            response = createHTTPResponse(generateRegisterPage("Ошибка при создании пользователя", username, ""));
                        }
                    }
                }
            } else {
                response = createHTTPResponse(generateRegisterPage("Ошибка: некорректные данные"));
            }
        } else if (request.find("POST /logout") == 0) {
            if (!sessionId.empty()) {
                db.deleteSession(sessionId);
            }
            response = "HTTP/1.1 302 Found\r\nLocation: /\r\nSet-Cookie: session_id=; Path=/; HttpOnly; Max-Age=0\r\nSet-Cookie: tab_token=; Path=/; Max-Age=0\r\nConnection: close\r\n\r\n";
        } else if (request.find("POST /add") == 0 && session && session->isAdmin) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                std::string body = request.substr(bodyStart + 4);
                auto params = parsePostData(body);
                
                std::string website = params["website"];
                int countryId = 0;
                if (!params["country_id"].empty()) {
                    try { countryId = std::stoi(params["country_id"]); } catch (...) {}
                }
                
                // Добавляем интегратора и получаем ID
                int newId = db.addIntegratorAndGetId(params["name"], params["city"], params["description"], website, countryId);
                
                if (newId > 0) {
                    // Добавляем лицензии
                    size_t pos = 0;
                    while ((pos = body.find("license_number[]=", pos)) != std::string::npos) {
                        pos += 17;
                        size_t end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        std::string num = urlDecode(body.substr(pos, end - pos));
                        pos = body.find("license_issued_by[]=", end);
                        if (pos != std::string::npos) {
                            pos += 20;
                            end = body.find("&", pos);
                            if (end == std::string::npos) end = body.length();
                            std::string issued = urlDecode(body.substr(pos, end - pos));
                            if (!num.empty() && !issued.empty()) {
                                db.addLicense(newId, num, issued);
                            }
                        }
                    }
                    
                    // Добавляем сертификаты
                    pos = 0;
                    while ((pos = body.find("certificate_name[]=", pos)) != std::string::npos) {
                        pos += 19;
                        size_t end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        std::string name = urlDecode(body.substr(pos, end - pos));
                        
                        pos = body.find("certificate_number[]=", end);
                        std::string number = "";
                        if (pos != std::string::npos) {
                            pos += 21;
                            end = body.find("&", pos);
                            if (end == std::string::npos) end = body.length();
                            number = urlDecode(body.substr(pos, end - pos));
                        }
                        
                        pos = body.find("certificate_issued_by[]=", end);
                        if (pos != std::string::npos) {
                            pos += 24;
                            end = body.find("&", pos);
                            if (end == std::string::npos) end = body.length();
                            std::string issued = urlDecode(body.substr(pos, end - pos));
                            if (!name.empty() && !issued.empty()) {
                                db.addCertificate(newId, name, number, issued);
                            }
                        }
                    }
                    
                    // Добавляем продукты
                    std::vector<int> productIds;
                    pos = 0;
                    while ((pos = body.find("products[]=", pos)) != std::string::npos) {
                        pos += 11;
                        size_t end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        try {
                            productIds.push_back(std::stoi(urlDecode(body.substr(pos, end - pos))));
                        } catch (...) {}
                    }
                    if (!productIds.empty()) {
                        db.setIntegratorProducts(newId, productIds);
                    }
                    
                    // Добавляем услуги
                    std::vector<int> serviceIds;
                    pos = 0;
                    while ((pos = body.find("services[]=", pos)) != std::string::npos) {
                        pos += 11;
                        size_t end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        try {
                            serviceIds.push_back(std::stoi(urlDecode(body.substr(pos, end - pos))));
                        } catch (...) {}
                    }
                    if (!serviceIds.empty()) {
                        db.setIntegratorServices(newId, serviceIds);
                    }
                }
            }
            response = createRedirectResponse("/");
        } else if (request.find("POST /update") == 0 && session && session->isAdmin) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                std::string body = request.substr(bodyStart + 4);
                auto params = parsePostData(body);
                
                int id = std::stoi(params["id"]);
                std::string website = params["website"];
                int countryId = 0;
                if (!params["country_id"].empty()) {
                    try { countryId = std::stoi(params["country_id"]); } catch (...) {}
                }
                
                // Обновляем интегратора
                db.updateIntegrator(id, params["name"], params["city"], params["description"], website, countryId);
                
                // Обновляем лицензии
                db.deleteLicenses(id);
                size_t pos = 0;
                while ((pos = body.find("license_number[]=", pos)) != std::string::npos) {
                    pos += 17;
                    size_t end = body.find("&", pos);
                    if (end == std::string::npos) end = body.length();
                    std::string num = urlDecode(body.substr(pos, end - pos));
                    pos = body.find("license_issued_by[]=", end);
                    if (pos != std::string::npos) {
                        pos += 20;
                        end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        std::string issued = urlDecode(body.substr(pos, end - pos));
                        if (!num.empty() && !issued.empty()) {
                            db.addLicense(id, num, issued);
                        }
                    }
                }
                
                // Обновляем сертификаты
                db.deleteCertificates(id);
                pos = 0;
                while ((pos = body.find("certificate_name[]=", pos)) != std::string::npos) {
                    pos += 19;
                    size_t end = body.find("&", pos);
                    if (end == std::string::npos) end = body.length();
                    std::string name = urlDecode(body.substr(pos, end - pos));
                    
                    pos = body.find("certificate_number[]=", end);
                    std::string number = "";
                    if (pos != std::string::npos) {
                        pos += 21;
                        end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        number = urlDecode(body.substr(pos, end - pos));
                    }
                    
                    pos = body.find("certificate_issued_by[]=", end);
                    if (pos != std::string::npos) {
                        pos += 24;
                        end = body.find("&", pos);
                        if (end == std::string::npos) end = body.length();
                        std::string issued = urlDecode(body.substr(pos, end - pos));
                        if (!name.empty() && !issued.empty()) {
                            db.addCertificate(id, name, number, issued);
                        }
                    }
                }
                
                // Обновляем продукты
                std::vector<int> productIds;
                pos = 0;
                while ((pos = body.find("products[]=", pos)) != std::string::npos) {
                    pos += 11;
                    size_t end = body.find("&", pos);
                    if (end == std::string::npos) end = body.length();
                    try {
                        productIds.push_back(std::stoi(urlDecode(body.substr(pos, end - pos))));
                    } catch (...) {}
                }
                db.setIntegratorProducts(id, productIds);
                
                // Обновляем услуги
                std::vector<int> serviceIds;
                pos = 0;
                while ((pos = body.find("services[]=", pos)) != std::string::npos) {
                    pos += 11;
                    size_t end = body.find("&", pos);
                    if (end == std::string::npos) end = body.length();
                    try {
                        serviceIds.push_back(std::stoi(urlDecode(body.substr(pos, end - pos))));
                    } catch (...) {}
                }
                db.setIntegratorServices(id, serviceIds);
            }
            response = createRedirectResponse("/");
        } else if (request.find("POST /delete") == 0 && session && session->isAdmin) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                std::string body = request.substr(bodyStart + 4);
                auto params = parsePostData(body);
                db.deleteIntegrator(std::stoi(params["id"]));
            }
            response = createRedirectResponse("/");
        } else if (request.find("POST /rate") == 0 && session) {
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                std::string body = request.substr(bodyStart + 4);
                auto params = parsePostData(body);
                int integratorId = std::stoi(params["id"]);
                int ratingVal = std::stoi(params["rating"]);
                ratingVal = std::max(1, std::min(5, ratingVal));
                std::string comment = params["comment"];
                db.addOrUpdateRating(integratorId, session->userId, ratingVal, comment);
            }
            response = createRedirectResponse("/");
        } else if (request.find("GET / ") == 0 || request.find("GET /?") == 0) {
            if (session) {
                std::string tabToken = getCookie(request, "tab_token");
                std::cout << "Пользователь: " << session->username << ", Admin: " << (session->isAdmin ? "Да" : "Нет") << ", Tab token: " << tabToken << std::endl;
                
                // Получаем параметры фильтрации и сортировки
                std::string cityParam = getQueryParam(request, "city");
                std::string filterCity = getQueryParam(request, "filter_city");
                std::string searchName = getQueryParam(request, "name");
                std::string sortOption = getQueryParam(request, "sort");
                if (sortOption.empty()) sortOption = "name_asc";
                if (sortOption != "name_asc" && sortOption != "name_desc" &&
                    sortOption != "city_asc" && sortOption != "city_desc" &&
                    sortOption != "rating_desc" && sortOption != "rating_asc") {
                    sortOption = "name_asc";
                }

                int page = 1;
                std::string pageParam = getQueryParam(request, "page");
                if (!pageParam.empty()) {
                    try { page = std::max(1, std::stoi(pageParam)); } catch (...) { page = 1; }
                }
                const int pageSize = 5;

                // Получаем данные
                std::vector<Integrator> integrators = db.getAllIntegrators();

                // Фильтрация по городу и названию
                std::vector<Integrator> filtered;
                for (const auto& itg : integrators) {
                    if (!filterCity.empty() && itg.city != filterCity) continue;
                    if (!cityParam.empty() && !containsCaseInsensitive(itg.city, cityParam)) continue;
                    if (!searchName.empty() && !containsCaseInsensitive(itg.name, searchName)) continue;
                    filtered.push_back(itg);
                }

                // Статистика рейтингов для сортировки и отображения
                std::map<int, RatingStats> ratingStats = db.getRatingStats();

                // Сортировка
                std::sort(filtered.begin(), filtered.end(), [&](const Integrator& a, const Integrator& b) {
                    if (sortOption == "name_desc") return a.name > b.name;
                    if (sortOption == "city_asc") return a.city < b.city;
                    if (sortOption == "city_desc") return a.city > b.city;
                    if (sortOption == "rating_desc") {
                        double ra = ratingStats.count(a.id) ? ratingStats[a.id].average : 0.0;
                        double rb = ratingStats.count(b.id) ? ratingStats[b.id].average : 0.0;
                        if (ra == rb) return a.name < b.name;
                        return ra > rb;
                    }
                    if (sortOption == "rating_asc") {
                        double ra = ratingStats.count(a.id) ? ratingStats[a.id].average : 0.0;
                        double rb = ratingStats.count(b.id) ? ratingStats[b.id].average : 0.0;
                        if (ra == rb) return a.name < b.name;
                        return ra < rb;
                    }
                    // default name_asc
                    return a.name < b.name;
                });

                // Пагинация
                int total = static_cast<int>(filtered.size());
                int totalPages = std::max(1, (total + pageSize - 1) / pageSize);
                if (page > totalPages) page = totalPages;
                int start = (page - 1) * pageSize;
                int end = std::min(start + pageSize, total);
                std::vector<Integrator> pageItems;
                for (int i = start; i < end; i++) pageItems.push_back(filtered[i]);

                // Рейтинги для текущей страницы
                std::map<int, std::vector<Rating>> integratorRatings;
                for (const auto& itg : pageItems) {
                    integratorRatings[itg.id] = db.getRatingsByIntegrator(itg.id);
                }

                std::vector<std::string> cities = db.getAllCities();
                std::vector<std::pair<int, std::string>> countries = db.getAllCountries();
                std::vector<std::pair<int, std::string>> products = db.getAllProducts();
                std::vector<std::pair<int, std::string>> services = db.getAllServices();
                response = createHTTPResponse(generateMainPage(pageItems, session->isAdmin, true, session->username, tabToken, cities, countries, products, services, cityParam, filterCity, searchName, sortOption, page, totalPages, total, ratingStats, integratorRatings));
            } else {
                response = createHTTPResponse(generateLoginPage());
            }
        } else if (request.find("GET /login_required") == 0) {
            response = createHTTPResponse(generateLoginPage("Требуется авторизация"));
        } else {
            response = createHTTPResponse(generateLoginPage());
        }
        
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
        
        delete session;
    }
    
    close(serverSocket);
    return 0;
}