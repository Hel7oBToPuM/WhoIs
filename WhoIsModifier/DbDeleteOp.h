#pragma once
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>

#include <iostream>

class DbDeleteOp {
    mongocxx::collection collection; // Коллекция, в которой хранятся домены в БД

public:
    void tryToDeleteDomain(const std::string& request_body); // Метод удаляющий домен
    explicit DbDeleteOp (const mongocxx::collection& _collection) : collection(_collection) {} // Конструктор
};