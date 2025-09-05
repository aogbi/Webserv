/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:42:43 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 17:04:00 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <poll.h>
#include <string>
#include <stdlib.h>
#include <netdb.h>
#include <sstream>
#include <errno.h>
#include <fstream>
#include <map>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctime>

extern char **environ;

// Global flag for graceful shutdown
extern volatile bool g_running;

// Forward declaration
class Request;

struct ClientConnection {
    int fd;
    std::string buffer;
    time_t lastActivity;
    bool requestComplete;
    
    ClientConnection() : fd(-1), lastActivity(0), requestComplete(false) {}
    ClientConnection(int socket_fd) : fd(socket_fd), lastActivity(time(NULL)), requestComplete(false) {}
};

struct Location {
    std::string path;
    std::vector<std::string> allowMethods;
    bool autoindex;
    std::string index;
    std::string root;
    std::string redirect;
    std::vector<std::string> cgiPath;
    std::vector<std::string> cgiExt;
    std::string uploadDir;
};

class Server {
private:
    std::string configfile;
    uint16_t port;
    int server_fd;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in address;
    socklen_t addrlen;
    
    // Configuration variables
    std::string serverName;
    std::string host;
    std::string root;
    std::string index;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::vector<Location> locations;

public:
    Server(std::string configfile);
    ~Server();

    int parseConfig();
    bool setup();
    int getSocket() const;
    int acceptClient();
    void run();
    
    // New methods for HTTP handling
    std::string handleRequest(const Request& req);
    std::string serveFile(const std::string& path, const Request& req);
    std::string generateDirectoryListing(const std::string& path, const std::string& requestPath);
    std::string getMimeType(const std::string& filename);
    Location* findLocation(const std::string& path);
    bool isMethodAllowed(const std::string& method, const Location* loc);
    
    // CGI methods
    bool isCgiRequest(const std::string& path, const Location* loc);
    std::string executeCgi(const std::string& scriptPath, const Request& req, const Location* loc);
    std::string getCgiInterpreter(const std::string& extension, const Location* loc);
    
    // File upload methods
    std::string handleFileUpload(const Request& req, const Location* loc);
    bool saveUploadedFile(const std::string& content, const std::string& filename, const std::string& uploadDir);
    
    // Helper methods
    std::string trim(const std::string& str);
    std::string removeSemicolon(const std::string& str);
    
    // Connection management methods
    void removeClient(std::vector<struct pollfd>& fds, std::map<int, ClientConnection>& clients, int index, int& nfds);
    void handleClientTimeout(std::vector<struct pollfd>& fds, std::map<int, ClientConnection>& clients, int& nfds);
    bool isRequestComplete(const std::string& buffer);
    void sendErrorResponse(int client_fd, int statusCode, const std::string& message);
};

#endif
