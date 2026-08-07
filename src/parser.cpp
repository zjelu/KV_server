#include "parser.hpp"
#include <sstream>
#include "log.hpp"

#include <sstream>
CommandType to_type(const std::string& name)
{
    static const std::unordered_map<std::string, CommandType> table{
        {"PING",   CommandType::PING},
        {"SIZE",   CommandType::SIZE},
        {"GET",    CommandType::GET},
        {"SET",    CommandType::SET},
        {"DEL",    CommandType::DEL},
        {"EXPIRE", CommandType::EXPIRE},
        {"EXISTS", CommandType::EXISTS}
    };

    auto it = table.find(name);
    return it == table.end() ? CommandType::INVALID : it->second;
}


Command Parser::parse(std::string& line) 
{
    std::istringstream ss(line);

    std::string command_name;
    ss >> command_name;

    CommandType type = to_type(command_name);

    if (type == CommandType::INVALID) {
        return Command{};
    }

    // 无参数命令
    if (type == CommandType::PING ||
        type == CommandType::SIZE)
    {
        std::string extra;

        if (ss >> extra) {
            return Command{};
        }

        return Command{type};
    }

    // 一个 key 的命令
    if (type == CommandType::GET ||
        type == CommandType::DEL ||
        type == CommandType::EXISTS)
    {
        std::string key;
        std::string extra;

        if (!(ss >> key) || (ss >> extra)) {
            return Command{};
        }

        return Command{type, key};
    }

    // SET key value
    if (type == CommandType::SET) {
        std::string key;
        std::string value;

        if (!(ss >> key >> value)) {
            return Command{};
        }

        std::string extra;
        if (ss >> extra) {
            return Command{};
        }

        return Command{type, key, value};
    }

    if (type == CommandType::EXPIRE) {
    std::string key;
    std::string seconds_text;
    std::string extra;

    if (!(ss >> key >> seconds_text)) {
        return Command{};
    }

    if (ss >> extra) {
        return Command{};
    }

    try {
        std::size_t pos = 0;
        long long seconds = std::stoll(seconds_text, &pos);

        if (pos != seconds_text.size()) {
            return Command{};
        }

        return Command{
            CommandType::EXPIRE,
            key,
            "",
            seconds
        };
    }
    catch (const std::exception&) {
        return Command{};
    }
}

    return Command{};
}

