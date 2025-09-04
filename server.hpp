/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:42:43 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/04 11:30:29 by aogbi            ###   ########.fr       */
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

class Server {
private:
    std::string configfile;
    uint16_t port;
    int server_fd;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_in address;
    socklen_t addrlen;

public:
    Server(std::string configfile);
    ~Server();

    int parseConfig();
    bool setup();
    int getSocket() const;
    int acceptClient();
    void run();
};

#endif