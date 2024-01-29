#pragma once
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>

#include <iostream>

class DbRenewOp {
    mongocxx::collection collection; // Коллекция, в которой хранятся домены в БД

public:
    void tryToRenewDomain(const std::string& request_body); // Метод изменяющий в БД время освобождения
    explicit DbRenewOp (const mongocxx::collection& _collection) : collection(_collection) {} // Конструктор
};
