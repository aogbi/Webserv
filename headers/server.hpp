/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:42:43 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:58:47 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <netdb.h>
#include <sstream>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.hpp"
#include "http_handler.hpp"
#include "cgi_handler.hpp"
#include "connection_manager.hpp"

// Global flag for graceful shutdown
extern volatile bool g_running;

class Server {
private:
    Config config;
    HttpHandler httpHandler;
    CgiHandler cgiHandler;
    ConnectionManager* connectionManager;
    
    int server_fd;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in address;
    socklen_t addrlen;

public:
    Server(const std::string& configFile);
    ~Server();

    bool setup();
    int getSocket() const;
    int acceptClient();
    void run();
};

#endif
