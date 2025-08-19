/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:46:51 by aogbi             #+#    #+#             */
/*   Updated: 2025/08/19 22:59:56 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

Server::Server(int port) : server_fd(-1), port(port) {
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
}

Server::~Server() {
    if (server_fd != -1) {
        close(server_fd);
        std::cout << "Server closed" << std::endl;
    }
}

bool Server::setup() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        return false;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return false;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        return false;
    }

    std::cout << "Server listening on port " << port << std::endl;
    return true;
}

int Server::getSocket() const {
    return server_fd;
}

void Server::run() {
    std::vector<struct pollfd> fds;
    struct pollfd server_poll;
    server_poll.fd = server_fd;
    server_poll.events = POLLIN;
    fds.push_back(server_poll);

    while (true) {
        int activity = poll(fds.data(), fds.size(), -1);
        if (activity < 0) {
            perror("poll failed");
            break;
        }

        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                std::cout << "New client connected: "
                          << inet_ntoa(client_addr.sin_addr)
                          << ":" << ntohs(client_addr.sin_port) << std::endl;

                struct pollfd client_poll;
                client_poll.fd = client_fd;
                client_poll.events = POLLIN;
                fds.push_back(client_poll);
            }
        }

        for (size_t i = 1; i < fds.size(); i++) {
            if (fds[i].revents & POLLIN) {
                char buffer[1024];
                int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) {
                    std::cout << "Client disconnected: " << fds[i].fd << std::endl;
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--;
                } else {
                    buffer[bytes] = '\0';
                    std::cout << "Request:\n" << buffer << std::endl;

                    const char* response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 38\r\n"
                        "\r\n"
                        "<html><h1>Hello, Webserv!</h1></html>";

                    send(fds[i].fd, response, strlen(response), 0);
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--;
                }
            }
        }
    }
}
