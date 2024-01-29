#include <algorithm>
#include <boost/optional/optional.hpp>

#include <bsoncxx/document/element.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <iomanip>
#include <chrono>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <csignal>

using bsoncxx::builder::basic::kvp; // инструмент создания bson строк
using bsoncxx::builder::basic::make_array; // инструмент создания bson массивов
using bsoncxx::builder::basic::make_document; // инструмент создания документов MongoDB
using bsoncxx::type; // enum с типами полей в bson документе

volatile sig_atomic_t exitFlag = 0; // флаг закрытия программы

int server_fd = 0; // дескриптор прослушивающего сокета
int client_fd = 0; // дескриптор клиентского сокета

// Обработчик сигналов
void signalHandler(int signal) {
    if (client_fd > 0) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    if (server_fd > 0) {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
    }

    exitFlag = 1;
    exit(1);
}

// Обработчик ошибок
void errorHandler(const std::string &error_str) {
    perror(error_str.c_str());
    kill(getpid(), SIGTERM);
}

// Получение запроса от клиента
bool receive(std::string &buf) {
    for (char c = '\0'; c != '\n';) {
        ssize_t count = recv(client_fd, &c, 1, 0);

        if (count == 1) {
            if (c != '\r' && c != '\n')
                buf.push_back(c);
        } else if (count == 0 || errno == ECONNREFUSED)
            return false;
        else {
            errorHandler("Receive error");
            return false;
        }
    }
    return true;
}

// Создание прослушивающего сокета
int createSocket() {
    int error;

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) errorHandler("Creating socket error");

    sockaddr_in info{AF_INET, htons(43), INADDR_ANY};

    error = bind(sock_fd, reinterpret_cast<sockaddr *>(&info), sizeof(info));
    if (error < 0) errorHandler("Binding error");

    error = listen(sock_fd, SOMAXCONN);
    if (error < 0) errorHandler("Listen error");

    return sock_fd;
}

std::string toUpperCase(std::string str) {
    for(char& c : str)
        c = std::toupper(c);
    return str;
}

std::string processDatabaseQuery (const boost::optional<bsoncxx::document::value>& result) {
    std::ostringstream response_stream; // поток в котором форматируется ответ
    if (result) {
        // Получение представления из ответа БД
        auto domain_info = result->view();
        // Обработка каждого поля из ответа БД
        for (const auto& element : domain_info) {
            switch (element.type()) {
                case type::k_utf8: // для строкового типа
                    response_stream << element.key() << " : "
                    << (element.key() == "domain" ? toUpperCase(element.get_utf8().value.to_string()) : element.get_utf8().value)
                    << std::endl;
                    break;
                case type::k_array: // для массива
                    for (auto ele : element.get_array().value)
                        response_stream << element.key() << " : " << ele.get_utf8().value << std::endl;
                    break;
                case type::k_date: // для даты
                    response_stream << element.key() << " : ";
                    std::time_t seconds_since_epoch = std::chrono::duration_cast<std::chrono::seconds>(element.get_date().value).count();
                    std::tm* time = std::gmtime(&seconds_since_epoch);
                    response_stream << std::put_time(time
                    , element.key() == "free-date" ? "%Y-%m-%d" : "%Y-%m-%dT%H:%M:%SZ") << std::endl;
                    break;
            }
        }
        return response_stream.str();
    }
    else
        return "No entries found for the selected source(s).";
}

int main() {
    // Задание обработчиков сигналов
    std::signal(SIGHUP, signalHandler);
    std::signal(SIGKILL, signalHandler);
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGQUIT, signalHandler);

    server_fd = createSocket(); // создаем сокет

    // Создание подключения и получение коллекции с доменами
    mongocxx::instance instance{};
    mongocxx::client client{mongocxx::uri{"mongodb://test:test@localhost:27017/?authSource=WhoIsDb"}};
    auto db = client["WhoIsDb"];
    auto collection = db["domains"];

    std::string domain; // домен получаемый от клиента
    std::string response; // ответ клиенту

    while (!exitFlag) {
        // Принятие подключений
        client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno != ECONNABORTED && errno != EAGAIN && errno != EWOULDBLOCK)
                errorHandler("Accept error");
            else
                continue;
        }

        if (!receive(domain)) continue; // получение домена от клиента

        auto find_result = collection.find_one({make_document(kvp("domain", domain))}); // Запрос в БД
        response = processDatabaseQuery(find_result); // Обработка ответа БД и получение итогового ответа для клиента

        send(client_fd, response.c_str(), response.size(), 0); // отправка результата клиенту

        // Закрытие соединения с клиентом
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);

        domain.clear(); // Очистка строки с запросом клиента
    }
    return 0;
}