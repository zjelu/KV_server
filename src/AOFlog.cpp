#include"AOFLog.hpp"
#include <sstream>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
 #include <fcntl.h>
#include <iostream>
#include <fstream>

bool AOFLog::is_open() const{
    return fd_>=0;
}

AOFLog::AOFLog(const std::string& path)
{
    fd_ = open(
        path.c_str(),
         O_RDWR | O_CREAT | O_APPEND,
        0644
    );

    if (fd_ == -1) {
        perror("open AOF");
    }
}


bool AOFLog::append(const std::string& record)
{
    //std::cerr << "ENTER AOFLog::append\n";
    std::string data = record;

    if (data.empty() || data.back() != '\n') {
        data.push_back('\n');
    }
    std::size_t written = 0;

    while (written < data.size()) {
        ssize_t n = write(
            fd_,
            data.data() + written,
            data.size() - written
        );

        if (n > 0) {
            written += static_cast<std::size_t>(n);
            continue;
        }

        if (n == -1 && errno == EINTR) {
            continue;
        }

        if (n == -1) {
            perror("write");
            return false;
        }

        // write() 返回 0：没有取得进展，避免死循环
        return false;
    }

    /*std::cerr << "[AOF::append] fd=" << fd_
          << " data=[" << data << "]"
          << " size=" << data.size()
          << '\n';*/

    return true;
}

bool AOFLog::replay(KVStore& store, Parser& parser,Executor& executor)
{
    if (lseek(fd_, 0, SEEK_SET) == -1) {
        perror("lseek");
        return false;
    }

    std::string contents;
    char buffer[4096];

    std::size_t out_read = 0;

       while (true) {
        ssize_t n = read(fd_, buffer, sizeof(buffer));

        if (n > 0) {
            contents.append(
                buffer,
                static_cast<std::size_t>(n)
            );
            continue;
        }

        if (n == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        perror("read");
        return false;
    }

    std::istringstream input(contents);
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        Command cmd = parser.parse(line);
        executor.execute(cmd);
    }

    if (lseek(fd_, 0, SEEK_END) == -1) {
        perror("lseek");
        return false;
    }

    return true;
}

AOFLog::~AOFLog()
{
    if (fd_ != -1) {
        close(fd_);
    }
}