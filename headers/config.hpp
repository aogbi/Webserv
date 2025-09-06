/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:58:47 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdint.h>

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
    
    Location() : autoindex(false) {}
};

class Config {
private:
    std::string configFile;
    uint16_t port;
    std::string serverName;
    std::string host;
    std::string root;
    std::string index;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::vector<Location> locations;
    
    // Helper methods
    std::string trim(const std::string& str);
    std::string removeSemicolon(const std::string& str);

public:
    Config(const std::string& configFile);
    
    // Parse configuration file
    bool parseConfig();
    
    // Getters
    uint16_t getPort() const { return port; }
    const std::string& getServerName() const { return serverName; }
    const std::string& getHost() const { return host; }
    const std::string& getRoot() const { return root; }
    const std::string& getIndex() const { return index; }
    const std::map<int, std::string>& getErrorPages() const { return errorPages; }
    size_t getClientMaxBodySize() const { return clientMaxBodySize; }
    const std::vector<Location>& getLocations() const { return locations; }
    
    // Location methods
    const Location* findLocation(const std::string& path) const;
    bool isMethodAllowed(const std::string& method, const Location* loc) const;
};

#endif // CONFIG_HPP
