#include "Connstate.hpp"

#include "log.hpp"

#include <sys/socket.h>
#include <cerrno>
#include <cstdio>

//负责直接接受fd发过来的原始字符串
void disable_write_event(int fd, int epollfd);

std::optional<std::string>
ConnState::pop_command_line()
{
    size_t pos = inbuf.find('\n');

    if (pos == std::string::npos) {
        return std::nullopt;
    }

    std::string line = inbuf.substr(0, pos);

    inbuf.erase(0, pos + 1);

    return line;
}


WriteResult ConnState::write_response()
{
    while (!outbuf.empty())
    {
        ssize_t n = send(
            fd,
            outbuf.data() ,
            outbuf.size(),
            0
        );
        last_active = time(nullptr);
        
        if (n > 0) 
        {
            outbuf.erase(0, n);
            continue;
        }
        else if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return WRITE_AGAIN;
            }

            if (errno == EINTR)
            {
                continue;
            }

            perror("send http");
            return WRITE_ERROR;
        }

        LOG_DEBUG("[WRITE] fd=%d sent=%zd  total=%zu",
              fd, n,  outbuf.size());
    }

    return WRITE_DONE;
}


void ConnState::reset_state()
{
    
}

