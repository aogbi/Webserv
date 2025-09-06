/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   connection_manager.cpp                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 11:02:11 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "connection_manager.hpp"
#include "response.hpp"
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <sys/socket.h>
#include <cstdlib>

ConnectionManager::ConnectionManager(int serverFd) {
    // Add server socket to poll list
    struct pollfd serverPoll;
    serverPoll.fd = serverFd;
    serverPoll.events = POLLIN;
    serverPoll.revents = 0;
    pollFds.push_back(serverPoll);
}

void ConnectionManager::addClient(int clientFd) {
    ClientConnection client(clientFd);
    clients[clientFd] = client;
    
    struct pollfd clientPoll;
    clientPoll.fd = clientFd;
    clientPoll.events = POLLIN;
    clientPoll.revents = 0;
    pollFds.push_back(clientPoll);
}

void ConnectionManager::removeClient(int clientFd) {
    // Remove from clients map
    clients.erase(clientFd);
    
    // Remove from poll fds
    for (std::vector<struct pollfd>::iterator it = pollFds.begin(); it != pollFds.end(); ++it) {
        if (it->fd == clientFd) {
            pollFds.erase(it);
            break;
        }
    }
    
    // Close the socket
    close(clientFd);
}

void ConnectionManager::handleTimeouts() {
    time_t currentTime = time(NULL);
    std::vector<int> clientsToRemove;
    
    for (std::map<int, ClientConnection>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (currentTime - it->second.lastActivity > CLIENT_TIMEOUT) {
            clientsToRemove.push_back(it->first);
        }
    }
    
    // Remove timed out clients
    for (size_t i = 0; i < clientsToRemove.size(); ++i) {
        std::cout << "Client " << clientsToRemove[i] << " timed out, removing..." << std::endl;
        removeClient(clientsToRemove[i]);
    }
}

bool ConnectionManager::isRequestComplete(const std::string& buffer) {
    // Check for end of headers
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        headerEnd = buffer.find("\n\n");
        if (headerEnd == std::string::npos) {
            return false; // Headers not complete
        }
        headerEnd += 2;
    } else {
        headerEnd += 4;
    }
    
    // Parse headers to check for Content-Length
    std::string headers = buffer.substr(0, headerEnd);
    size_t contentLengthPos = headers.find("Content-Length:");
    if (contentLengthPos == std::string::npos) {
        contentLengthPos = headers.find("content-length:");
    }
    
    if (contentLengthPos != std::string::npos) {
        // Extract Content-Length value
        size_t valueStart = headers.find(':', contentLengthPos) + 1;
        size_t valueEnd = headers.find('\r', valueStart);
        if (valueEnd == std::string::npos) {
            valueEnd = headers.find('\n', valueStart);
        }
        
        std::string lengthStr = headers.substr(valueStart, valueEnd - valueStart);
        
        // Trim whitespace
        while (!lengthStr.empty() && lengthStr[0] == ' ') lengthStr.erase(0, 1);
        while (!lengthStr.empty() && lengthStr[lengthStr.length() - 1] == ' ') lengthStr.erase(lengthStr.length() - 1);
        
        int contentLength = atoi(lengthStr.c_str());
        size_t totalExpectedLength = headerEnd + contentLength;
        
        return buffer.length() >= totalExpectedLength;
    }
    
    // No Content-Length header, request is complete after headers
    return true;
}

void ConnectionManager::sendErrorResponse(int clientFd, int statusCode, const std::string& message) {
    Response response;
    response.setStatus(statusCode, message);
    response.setContentType("text/html");
    
    std::ostringstream body;
    body << "<html><head><title>Error " << statusCode << "</title></head><body>";
    body << "<h1>Error " << statusCode << "</h1>";
    body << "<p>" << message << "</p>";
    body << "<hr><p>Webserv/1.0</p></body></html>";
    
    response.setBody(body.str());
    std::string responseStr = response.toString();
    
    send(clientFd, responseStr.c_str(), responseStr.length(), 0);
}

void ConnectionManager::updateClientActivity(int clientFd) {
    std::map<int, ClientConnection>::iterator it = clients.find(clientFd);
    if (it != clients.end()) {
        it->second.lastActivity = time(NULL);
    }
}
