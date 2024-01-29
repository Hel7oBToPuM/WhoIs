#include "DbModifyingOps.h"

using bsoncxx::builder::basic::kvp; // Инструмент создания bson строк
using bsoncxx::builder::basic::make_document; // Инструмент создания документов MongoDB

bool DbModifyingOps::processArrayInput(const std::string& string_array, std::vector<std::string>& dest) {
    std::istringstream array_stream(string_array); // Массив в строковом потоке
    std::string value; // Значение каждого элемента массива
    while (std::getline(array_stream, value, ',')) { // Пока элементы присутствуют в потоке считывается
        value.erase(0, value.find_first_not_of(' ')); // Удаляются пробелы до элемента массива
        value.erase(value.find_last_not_of(' ') + 1); // Удаляются пробелы после элемента массива
        if (value.empty()) return false;  // Если считывается пустота, возвращается false
        for (const char& c : value) // Каждый символ ввода
            if (!(std::isalpha(c) || std::isdigit(c) || c == '-' || c == '.')) // Если символ не буква, число, дефис, точка возвращается false
                return false;
        dest.push_back(value); // Добавляется элемент в итоговый массив
    }
    return true;
}

void DbModifyingOps::processUserRequest(const std::string &request_body) {
    std::istringstream request(request_body); // Строковый поток для обработки ввода
    std::string flag, value; // Флаг, значение флага
    while (request >> flag) { // Пока в потоке есть флаг и значения
        if (flag[0] == '-') { // Считывание флага
            if (requiredOptions.contains(flag) || optionalOptions.contains(flag)) { // Проверка существования такого флага
                if (arrayValueOptions.contains(flag)) { // Если флаг имеет значение в виде массива
                    std::vector<std::string> array; // Массив для значений флага
                    char start = '\0', end = '\0'; // Переменные для начального и конечного символа массива '[', ']'
                    std::getline(request >> start, value, ']'); // Считывается начальный символ массива и значения массива
                    request.seekg(-1, std::ios_base::cur); // Перемещается курсор в потоке на символ назад
                    request.get(end); // Извлекается конечный символ массива
                    if (start == '[' && end == ']' && processArrayInput(value, array)) // Если массив введен верно, обрабатываются значения внутри массива
                        user_kv_input[flag] = array; // Добавление флаг-массив
                    else
                        throw "Invalid input for "+flag+'!';
                } else {
                    std::string value_part; // Часть введенного значения для флага
                    while (std::getline(request, value_part, '-')) { // Пока не будет считано все значение
                        value += value_part; // Добавление в общее значение для флага
                        if (!request.eof()) { // Если в потоке остались данные
                            request.seekg(-1, std::ios_base::cur); // Перемещается курсор в потоке на символ назад
                            request >> value_part; // Считывается следующая часть значения
                            if (value.back() == ' ' && (requiredOptions.contains(value_part) || optionalOptions.contains(value_part))) { // Если считанная часть является следующий флагом
                                request.seekg(-value_part.size(), std::ios_base::cur); // Перемещается курсор в потоке до флага
                                break;
                            }
                            value += value_part; //Добавление в общее значения для флага
                        }
                    }
                    value.erase(0, value.find_first_not_of(' ')); // Удаляются пробелы до элемента массива
                    value.erase(value.find_last_not_of(' ') + 1); // Удаляются пробелы после элемента массива
                    if (!value.empty()) // Если что-то было считано
                        user_kv_input[flag] = value; // Добавление флаг-значение
                    else throw "Missing value for "+flag+'!';
                }
            } else throw "Unknown option "+flag+'!';
        } else throw "Value \""+flag+"\" without qualifying option!";
        value.clear(); // Очистка переменной значения
    }
}

void DbModifyingOps::processDateStrings(const std::string &operationToDb = "add") {
    using std::chrono::system_clock; // Для сокращенной записи типа system_clock
    std::tm utc_time = {}; // Переменная для хранения даты в формате tm
    for (const auto &flag: timeValueOptions) { // Для каждого флага из всех флагов со значением времени
        if (user_kv_input.contains(flag)) { // Если пользователь ввел значения для флага
            std::istringstream time_string(std::get<std::string>(user_kv_input[flag])); // Передача строковой даты в строковый поток
            if (time_string >> std::get_time(&utc_time, (flag == "-fd" ? "%Y-%m-%d" : " %Y-%m-%dT%H:%M:%SZ"))) // Чтение даты по формату
                user_kv_input[flag] = system_clock::from_time_t(timegm(&utc_time)); // Запись даты в массив флаг-значение
            else
                throw "Invalid time value format for flag "+flag+'!';
        } else {
            std::time_t time; // Представление времени в формате секунд с начала эпохи Unix
            if (flag == "-cr" && operationToDb == "add") { // Если пользователь указал флаг -cr и операция добавления в БД
                time = system_clock::to_time_t(system_clock::now()); // Получение сегодняшней даты
                user_kv_input[flag] = system_clock::from_time_t(timegm(localtime(&time))); // Запись даты в массив флаг-значение
            } else if (flag == "-pt" && user_kv_input.contains("-cr")) { // Если пользователь указал флаг -pt и был уточнено время создания
                time = system_clock::to_time_t(std::get<system_clock::time_point>(user_kv_input["-cr"])); // Получение времени создания
                utc_time = *gmtime(&time); // Преобразование в удобный формат для редактирования
                utc_time.tm_year += 1; // Добавление года
                user_kv_input[flag] = system_clock::from_time_t(timegm(&utc_time)); // Запись даты в массив флаг-значение
            } else if (flag == "-fd" && user_kv_input.contains("-pt")) { // Если пользователь указал флаг -fd и был уточнено время окончания периода
                time = system_clock::to_time_t(std::get<system_clock::time_point>(user_kv_input["-pt"]));
                utc_time = *gmtime(&time); // Преобразование в удобный формат для редактирования
                utc_time.tm_mon += 1; // Добавление месяца
                user_kv_input[flag] = system_clock::from_time_t(timegm(&utc_time)); // Запись даты в массив флаг-значение
            }
        }
    }
}

bsoncxx::document::value DbModifyingOps::getDbView() {
    using bsoncxx::builder::basic::sub_array; // Инструмент для создания массива-значения
    auto document = bsoncxx::builder::basic::document{}; // Билдер документа
    for (const auto &pair: user_kv_input) { // Каждая пара флаг-значение
        if (const auto *strValue = std::get_if<std::string>(&pair.second)) { // Если хранится строка
            document.append(kvp(flagToField.at(pair.first), *strValue)); // Добавление ключ-строка
        } else if (const auto *vecValue = std::get_if<std::vector<std::string>>(&pair.second)) { // Если хранится массив строк
            document.append(kvp(flagToField.at(pair.first), [&vecValue](sub_array child) { // Добавление ключ-массив
                for (const auto &element: *vecValue) // Для каждого элемента массива
                    child.append(element); // Добавление элемента представление массива БД
            }));
        } else if (const auto *timeValue = std::get_if<std::chrono::system_clock::time_point>(&pair.second)) { // Если хранится дата
            document.append(kvp(flagToField.at(pair.first), bsoncxx::types::b_date(*timeValue))); // Добавление ключ-дата
        }
    }
    return document.extract();
}

void DbModifyingOps::tryAddDomain(const std::string &request_body) {
    try {
        processUserRequest(request_body); // Обработка ввода пользователя
        for (const std::string &flag: requiredOptions) // Для каждого требуемого флага
            if (!user_kv_input.contains(flag)) // Если требуемый флаг отсутствуют
                throw "Required option" + flag + "is missing!";
        processDateStrings(); // Обработка строкового формата даты в формат C++
        if (!collection.insert_one(getDbView())) // Если не добавилось значение в БД
            throw "Failed to add domain!";
        std::cout << "Domain added!";
    } catch (const std::string& error) {
        std::cerr << error;
    }
    user_kv_input.clear(); // Очистка массива флаг-значение
}

void DbModifyingOps::tryModifyDomain(const std::string &request_body) {
    try {
        std::istringstream request(request_body); // Помещение ввода пользователя в строковый поток
        std::string domain, values; // Домен, значения
        std::getline(request >> domain, values); // Получение домена и флагов-значений
        auto find_result = collection.find_one({make_document(kvp("domain", domain))}); // Поиск в БД домена
        if (find_result) { // Если есть такой домен
            processUserRequest(values); // Обработка ввода пользователя
            processDateStrings("modify"); // Обработка строкового формата даты в формат C++
            auto update_one_result = collection.update_one(make_document(kvp("domain", domain)) // Изменение значений в БД
                    , make_document(kvp("$set", getDbView())));
            if (!update_one_result) // Если изменения не были добавлены
                throw "Failed to modify domain!";
            std::cout << "Domain modified!";
        } else
            throw "Wrong domain name!";
    } catch (const char* error) {
        std::cerr << error << std::endl;
    }
    user_kv_input.clear(); // Очистка массива флаг-значение
}
