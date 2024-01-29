#pragma once
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>

#include <istream>
#include <map>
#include <set>
#include <iostream>

class DbModifyingOps {

    // Компаратор для определения порядка флагов в массивах
    struct KeyComparator {
        bool operator()(const std::string &key1, const std::string &key2) const {
            std::map<std::string, int> keyOrder = {
                    {"-d",   1}, {"-ns",  2},
                    {"-st",  3}, {"-org", 4},
                    {"-reg", 5}, {"-cr",  6},
                    {"-pt",  7}, {"-fd",  8},
                    {"-src", 9}
            };
            if (keyOrder.count(key1) == 0 || keyOrder.count(key2) == 0) // Возвращаем false для неопределенных ключей
                return false;
            return keyOrder[key1] < keyOrder[key2]; // Возвращаем в порядке возрастания
        }
    };

    const std::set<std::string, KeyComparator> requiredOptions = {"-d", "-ns", "-st", "-org", "-reg", "-src"}; // Требуемые флаги-поля домена в БД
    const std::set<std::string, KeyComparator> optionalOptions = {"-cr", "-pt", "-fd"}; // Необязательные флаги-поля домена в БД
    const std::set<std::string, KeyComparator> arrayValueOptions = {"-ns"}; // Флаги-поля домена в БД, принимающие на вход массив формата [value, value]
    const std::set<std::string, KeyComparator> timeValueOptions = {"-cr", "-pt", "-fd"}; // Флаги-поля домена в БД принимающие на вход дату формата "%Y-%m-%d" и " %Y-%m-%dT%H:%M:%SZ"

    const std::map<std::string, std::string, KeyComparator> flagToField = { // Сопоставление флага и поля домена в БД
            {"-d",   "domain"}, {"-ns",  "nserver"},
            {"-st",  "state"}, {"-org", "org"},
            {"-reg", "registrar"}, {"-cr",  "created"},
            {"-pt",  "paid-till"}, {"-fd",  "free-date"},
            {"-src", "source"}
    };

    mongocxx::collection collection; // Коллекция, в которой хранятся домены в БД

    std::map<std::string, std::variant<std::string, std::vector<std::string>, std::chrono::system_clock::time_point>, KeyComparator> user_kv_input; // Флаг-значения пользовательского ввода

    bsoncxx::document::value getDbView(); // Метод обрабатывающий массив флаг-значение и преобразующий его в БД представление
    void processDateStrings(const std::string& operationToDb); // Метод обрабатывающий строковый формат даты в формат представления C++
    bool processArrayInput(const std::string& string_array, std::vector<std::string>& dest); // Метод обрабатывающий значения массива
    void processUserRequest(const std::string& request_body); // Метод обрабатывающий ввод пользователя и преобразующий его в ключ-значение

public:

    explicit DbModifyingOps(const mongocxx::collection& _collection) : collection(_collection) {} // Конструктор

    void tryAddDomain(const std::string& request_body); // Метод добавляющий в БД домен
    void tryModifyDomain(const std::string& request_body); // Метод изменяющий в БД домен
};

