#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5008);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        std::cerr << "inet_pton failed\n";
        close(fd);
        return 1;
    }

    if (connect(
            fd,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)
        ) == -1)
    {
        perror("connect");
        close(fd);
        return 1;
    }

while(true){
    std::string request;

    if (!std::getline(std::cin, request)) {
        break;  // stdin 到达 EOF 或发生错误
    }

    if (request == "quit") {
        break;
    }

    request += '\n';

     std::cout << "[client] request size = "
          << request.size() << '\n';

     std::cout << "[client] sending: ["<< request << "]\n";

    if(request=="quit") break;

    send(
        fd,
        request.data(),
        request.size(),
        0
    );

    char buf[1024];

    ssize_t n = recv(fd, buf, sizeof(buf), 0);

    if (n > 0) {
        std::string response(buf, n);

        std::cout << "[client] response: "<< response;
    }

}

    close(fd);

    return 0;
}