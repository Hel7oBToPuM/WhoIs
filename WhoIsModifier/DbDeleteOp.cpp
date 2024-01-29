#include "DbDeleteOp.h"

using bsoncxx::builder::basic::kvp; // Инструмент создания bson строк
using bsoncxx::builder::basic::make_document; // Инструмент создания документов MongoDB

void DbDeleteOp::tryToDeleteDomain(const std::string &request_body) {
    try {
        std::istringstream request(request_body); // Помещение ввода пользователя в строковый поток
        std::string domain; // Домен
        request >> domain; // Считывание домена
        if (!(request >> domain)) {// Если ввод корректен
            if (!collection.delete_one(make_document(kvp("domain", domain)))) // Если удаление из БД не удалось
                throw "Wrong domain name!";
        }
        else
            throw "Request must contain only domain name!";
        std::cout << "Domain deleted!";
    } catch (const char* error) {
        std::cerr << error << std::endl;;
    }
}
