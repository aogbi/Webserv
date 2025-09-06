/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_manager.hpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 11:02:11 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_MANAGER_HPP
#define CONNECTION_MANAGER_HPP

#include <map>
#include <vector>
#include <poll.h>
#include <ctime>
#include <string>

struct ClientConnection {
    int fd;
    std::string buffer;
    time_t lastActivity;
    bool requestComplete;
    
    ClientConnection() : fd(-1), lastActivity(0), requestComplete(false) {}
    ClientConnection(int socket_fd) : fd(socket_fd), lastActivity(time(NULL)), requestComplete(false) {}
};

class ConnectionManager {
private:
    std::map<int, ClientConnection> clients;
    std::vector<struct pollfd> pollFds;
    static const int CLIENT_TIMEOUT = 30; // seconds
    
public:
    ConnectionManager(int serverFd);
    
    /**
     * @brief Add a new client connection
     * @param clientFd The client socket file descriptor
     */
    void addClient(int clientFd);
    
    /**
     * @brief Remove a client connection
     * @param clientFd The client socket file descriptor
     */
    void removeClient(int clientFd);
    
    /**
     * @brief Get the poll file descriptors
     * @return Reference to the poll fds vector
     */
    std::vector<struct pollfd>& getPollFds() { return pollFds; }
    
    /**
     * @brief Get clients map
     * @return Reference to the clients map
     */
    std::map<int, ClientConnection>& getClients() { return clients; }
    
    /**
     * @brief Handle client timeouts
     */
    void handleTimeouts();
    
    /**
     * @brief Check if HTTP request is complete
     * @param buffer The request buffer
     * @return true if request is complete
     */
    bool isRequestComplete(const std::string& buffer);
    
    /**
     * @brief Send error response to client
     * @param clientFd Client socket file descriptor
     * @param statusCode HTTP status code
     * @param message Error message
     */
    void sendErrorResponse(int clientFd, int statusCode, const std::string& message);
    
    /**
     * @brief Update client activity timestamp
     * @param clientFd Client socket file descriptor
     */
    void updateClientActivity(int clientFd);
};

#endif // CONNECTION_MANAGER_HPP
