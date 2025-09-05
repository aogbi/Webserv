/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_fixed.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:42:43 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:29:16 by aogbi            ###   ########.fr       */
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

extern char **environ;

// Forward declaration
class Request;

struct Location {
    std::string path;
    std::vector<std::string> allowMethods;
    bool autoindex;
    std::string index;
    std::string root;
    std::string redirect;
    std::vector<std::string> cgiPath;
    std::vector<std::string> cgiExt;
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
};

#endif
