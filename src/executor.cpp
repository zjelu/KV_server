#include"executor.hpp"

Executor::Executor(KVStore& store)
    : store_(store)
{
}

std::string Executor::execute(const Command& cmd)
{
    switch (cmd.type) {
        case CommandType::PING:
            return "PONG\n";

        case CommandType::SET:
            store_.set(cmd.key, cmd.value);
            return "OK\n";

        case CommandType::GET: {
            auto value = store_.get(cmd.key);

            if (!value) {
                return "NOT_FOUND\n";
            }

            return *value + "\n";
        }

        case CommandType::DEL: {
            Status st= store_.del(cmd.key);

            if (st== Status::OK) {
                return "OK\n";
            }

            return "NOT FOUND\n";
        }
        case CommandType::EXISTS: {
            bool st= store_.exists(cmd.key);

           if (st) {
                return "OK\n";
            }

            return "NOT FOUND\n";
        }
        case CommandType::SIZE: {

            return std::to_string(store_.size());
        }
        

        case CommandType::EXPIRE: {
        Status status = store_.expire( cmd.key,std::chrono::seconds(cmd.seconds));

        if (status == Status::OK) {
            return "OK\n";
        }

        if (status == Status::NOT_FOUND) {
            return "NOT_FOUND\n";
        }

        return "ERROR\n";
        }

        case CommandType::INVALID:
            return "ERROR invalid command\n";

        default:
            return "ERROR\n";
    }
}