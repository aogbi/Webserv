/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 12:26:14 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "request.hpp"
#include "response.hpp"
#include <poll.h>
#include <sys/socket.h>
#include <cstring>
#include <cerrno>
#include <stdexcept>

Server::Server(const std::string& configFile) 
    : config(configFile), httpHandler(config), cgiHandler(config), connectionManager(NULL), server_fd(-1) {
    
    if (!config.parseConfig()) {
        std::cerr << "Failed to parse configuration file" << std::endl;
        throw std::runtime_error("Failed to parse configuration file");
    }
    
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    std::stringstream ss;
    ss << config.getPort();
    std::string portStr = ss.str();
    
    if (getaddrinfo(NULL, portStr.c_str(), &hints, &res) != 0) {
        std::cerr << "getaddrinfo failed: " << gai_strerror(errno) << std::endl;
        throw std::runtime_error("getaddrinfo failed");
    }
    
    address = *(struct sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);
}

Server::~Server() {
    if (connectionManager) {
        delete connectionManager;
    }
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

    // Initialize connection manager
    connectionManager = new ConnectionManager(server_fd);
    
    std::cout << "Server listening on port " << config.getPort() << std::endl;
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
    const int POLL_TIMEOUT = 1000; // 1 second poll timeout
    
    std::cout << "🚀 Server running... (Press Ctrl+C to stop)" << std::endl;

    while (g_running) {
        // Handle client timeouts periodically
        connectionManager->handleTimeouts();
        
        std::vector<struct pollfd>& fds = connectionManager->getPollFds();
        int activity = poll(&fds[0], fds.size(), POLL_TIMEOUT);
        
        if (activity < 0) {
            if (errno == EINTR && !g_running) {
                break; // Interrupted by signal, check g_running flag
            }
            std::cerr << "❌ Poll error: " << strerror(errno) << std::endl;
            break;
        }
        
        if (activity == 0) {
            continue; // Timeout, check g_running flag and continue
        }

        // Check for new connections on server socket
        if (fds[0].revents & POLLIN) {
            int client_fd = acceptClient();
            if (client_fd != -1) {
                connectionManager->addClient(client_fd);
                std::cout << "📝 Client connected (fd: " << client_fd << ")" << std::endl;
            }
        }

        // Handle existing client connections
        std::map<int, ClientConnection>& clients = connectionManager->getClients();
        
        for (size_t i = 1; i < fds.size() && g_running; i++) {
            int client_fd = fds[i].fd;
            
            // Handle client disconnection or errors
            if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                std::cout << "📤 Client disconnected (fd: " << client_fd << ")" << std::endl;
                connectionManager->removeClient(client_fd);
                i--; // Adjust index after removal
                continue;
            }
            
            // Handle incoming data
            if (fds[i].revents & POLLIN) {
                char buffer[4096] = {0};
                ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                
                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
                        std::cout << "📤 Client closed connection (fd: " << client_fd << ")" << std::endl;
                    } else {
                        std::cerr << "❌ Read error on client " << client_fd << ": " << strerror(errno) << std::endl;
                    }
                    connectionManager->removeClient(client_fd);
                    i--; // Adjust index after removal
                    continue;
                }
                
                // Update client activity and append to buffer
                connectionManager->updateClientActivity(client_fd);
                clients[client_fd].buffer.append(buffer, bytes_read);
                
                // Check if we have a complete request
                if (connectionManager->isRequestComplete(clients[client_fd].buffer)) {
                    clients[client_fd].requestComplete = true;
                    
                    // Parse and handle the request
                    Request req;
                    if (req.parse(clients[client_fd].buffer)) {
                        std::string responseStr;
                        
                        // Check if this is a CGI request
                        const Location* location = config.findLocation(req.getPath());
                        if (cgiHandler.isCgiRequest(req.getPath(), location)) {
                            responseStr = cgiHandler.executeCgi(req.getPath(), req, location);
                        } else {
                            responseStr = httpHandler.handleRequest(req);
                        }
                        
                        ssize_t sent = send(client_fd, responseStr.c_str(), responseStr.length(), MSG_NOSIGNAL);
                        if (sent < 0) {
                            std::cerr << "❌ Send error to client " << client_fd << ": " << strerror(errno) << std::endl;
                        }
                    } else {
                        connectionManager->sendErrorResponse(client_fd, 400, "Bad Request");
                    }
                    
                    // Close connection after response (HTTP/1.0 behavior)
                    std::cout << "📤 Request completed, closing connection (fd: " << client_fd << ")" << std::endl;
                    connectionManager->removeClient(client_fd);
                    i--; // Adjust index after removal
                }
            }
        }
    }
    
    std::cout << "✅ Server shutdown complete." << std::endl;
}
