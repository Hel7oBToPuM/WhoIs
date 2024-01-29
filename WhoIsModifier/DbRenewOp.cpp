#include "DbRenewOp.h"

using bsoncxx::builder::basic::kvp; // Инструмент создания bson строк
using bsoncxx::builder::basic::make_document; // Инструмент создания документов MongoDB

void DbRenewOp::tryToRenewDomain(const std::string &request_body) {
    try {
        std::istringstream request (request_body); // Помещение ввода пользователя в строковый поток
        std::string domain, temp; // Домен, временная переменная
        std::chrono::system_clock::time_point pt_date, fd_date; // Время окончания периода, время освобождения
        request >> domain; // Получение домена из строкового потока
        auto find_result = collection.find_one({make_document(kvp("domain", domain))}); // Поиск в БД домена
        if (find_result) { // Если есть такой домен
            auto domain_info = find_result->view(); // Получение read-write представления результата поиска
            std::tm tm_date{}; // Переменная для хранения даты в формате tm
            if (!(request >> std::get_time(&tm_date, "%Y-%m-%dT%H:%M:%SZ"))) { // Если формат верный
                if (!(request >> temp)) { // Если ввод корректен
                    std::time_t seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(domain_info["paid-till"].get_date().value).count(); // Время с начала Unix эпохи в секундах
                    tm_date = *std::gmtime(&seconds_since_epoch); // Преобразование в удобный формат для редактирования
                    tm_date.tm_year += 1; // Добавление года
                } else
                    throw "Incorrect new date input!";
            }
            pt_date = std::chrono::system_clock::from_time_t(timegm(&tm_date)); // Задание времени окончания периода
            tm_date.tm_mon += 1; // Добавление месяца
            fd_date = std::chrono::system_clock::from_time_t(timegm(&tm_date)); // Задание времени освобождения
        } else
            throw domain+" domain is not registered!";
        auto update_one_result = collection.update_one(make_document(kvp("domain", domain)), make_document( // Изменение значений в БД
                kvp("$set", make_document(
                        kvp("paid-till", bsoncxx::types::b_date(pt_date)),
                        kvp("free-date", bsoncxx::types::b_date(fd_date))))));
        if (!update_one_result) // Если изменения не были добавлены
            throw "Failed to renew domain!";
        std::cout << "Paid-till date updated!";
    } catch (const char* error) {
        std::cerr << error << std::endl;;
    }
}
