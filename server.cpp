/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:46:51 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/04 11:51:30 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "request.hpp"

Server::Server(std::string file) : configfile(file) {
	parseConfig();
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	std::stringstream ss;
	ss << port;
	std::string portStr = ss.str();
	if (getaddrinfo(NULL, portStr.c_str(), &hints, &res) != 0) {
		std::cerr << "getaddrinfo failed: " << gai_strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	address = *(struct sockaddr_in *)res->ai_addr;
	freeaddrinfo(res);
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
		std::cerr << "socket failed: " << strerror(errno) << std::endl;
		return false;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		std::cerr << "bind failed: " << strerror(errno) << std::endl;
		return false;
	}

	if (listen(server_fd, SOMAXCONN) < 0) {
		std::cerr << "listen failed: " << strerror(errno) << std::endl;
		return false;
	}

	std::cout << "Server listening on port " << port << std::endl;
	return true;
}

int Server::getSocket() const {
	return server_fd;
}
int Server::acceptClient() {
	addrlen = sizeof(address);
	int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
	if (client_fd < 0) {
		std::cerr << "accept failed: " << strerror(errno) << std::endl;
		return -1;
	}
	std::cout << "New connection accepted" << std::endl;
	return client_fd;
}

void Server::run() {
	struct pollfd fds[200];
	fds[0].fd = server_fd;
	fds[0].events = POLLIN;
	int nfds = 1;

	while (true) {
		int activity = poll(fds, nfds, -1);
		if (activity < 0) {
			std::cerr << "poll error: " << strerror(errno) << std::endl;
			break;
		}

		if (fds[0].revents & POLLIN) {
			int client_fd = acceptClient();
			if (client_fd != -1 && nfds < 200) {
				fds[nfds].fd = client_fd;
				fds[nfds].events = POLLIN;
				nfds++;
			}
		}

		for (int i = 1; i < nfds; i++) {
			if (fds[i].revents & POLLIN) {
				char buffer[1024] = {0};
				int bytes_read = read(fds[i].fd, buffer, sizeof(buffer));
				if (bytes_read <= 0) {
					close(fds[i].fd);
					std::cout << "Connection closed" << std::endl;
					fds[i] = fds[nfds - 1];
					nfds--;
					i--;
				} else {
					std::cout << "Received: " << buffer << std::endl;
					Request req;
					if (req.parse(std::string(buffer, bytes_read))) {
						std::cout << "Method: " << req.getMethod() << std::endl;
						std::cout << "Path: " << req.getPath() << std::endl;
						std::cout << "Version: " << req.getVersion() << std::endl;
						std::cout << "Headers:" << std::endl;
						const std::map<std::string, std::string>& hdrs = req.getHeaders();
						for (std::map<std::string, std::string>::const_iterator it = hdrs.begin(); it != hdrs.end(); ++it) {
							std::cout << it->first << ": " << it->second << std::endl;
						}
						std::cout << "Body: " << req.getBody() << std::endl;
					}
					const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
					send(fds[i].fd, response, strlen(response), 0);
				}
			}
		}
	}
}

int Server::parseConfig() {
	port = 8080;
	return 0;
}
