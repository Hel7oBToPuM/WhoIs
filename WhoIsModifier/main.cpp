#include "DbDeleteOp.h"
#include "DbModifyingOps.h"
#include "DbRenewOp.h"
#include <mongocxx/instance.hpp>

#include <iostream>

//add -d dfdf -ns [ addfdf.rt, dfdf.m] -st dfdf -org dfdf -reg dfdf -src dfdfdf
//modify dfdf -ns [ addfdf.rt, dfdf.m] -st dfdf -org dfdf -reg dfdf -cr 2020-02-12T20:00:00Z -src dfdfdf
//renew dfdf
//delete dfdf

int main() {
    std::string request, request_body; // Запрос, тело запроса
    mongocxx::instance instance{}; // Создание инстанса подключения к БД
    mongocxx::client client{mongocxx::uri{"mongodb://test:test@localhost:27017/?authSource=WhoIsDb"}}; // Подключение к серверу БД
    auto db = client["WhoIsDb"]; // Подключение к БД
    mongocxx::collection collection = db["domains"]; // Получение коллекции, в которой хранятся все домены
    DbModifyingOps mod_ops(collection); // Создание объекта модификации и добавления домена в БД
    DbRenewOp renew_op(collection); // Создание объекта обновления времени освобождения в БД
    DbDeleteOp delete_op(collection); // Создание объекта удаления домена из БД
    while(true) {
        std::cin >> request; // Получение команды от пользователя
        std::getline(std::cin, request_body); // Получение запроса
        if (request == "add")
            mod_ops.tryAddDomain(request_body); // Добавление
        else if (request == "modify") // Модификация
            mod_ops.tryModifyDomain(request_body);
        else if (request == "renew") // Обновление времени освобождения
            renew_op.tryToRenewDomain(request_body);
        else if (request == "delete") // Удаление
            delete_op.tryToDeleteDomain(request_body);
        else
            std::cout << "Unknown command!" << std::endl; // Ошибка
    }
}