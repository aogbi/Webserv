/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:58:47 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config.hpp"

Config::Config(const std::string& configFile) 
    : configFile(configFile), port(8080), serverName("localhost"), 
      host("127.0.0.1"), root("./"), index("index.html"), clientMaxBodySize(1000000) {
}

bool Config::parseConfig() {
    std::ifstream file(configFile.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << configFile << std::endl;
        return false;
    }
    
    std::string line;
    Location currentLocation;
    bool inLocationBlock = false;
    
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        std::istringstream iss(line);
        std::string directive;
        iss >> directive;
        
        if (directive == "server") {
            continue;
        } else if (directive == "location") {
            if (inLocationBlock) {
                locations.push_back(currentLocation);
            }
            currentLocation = Location();
            iss >> currentLocation.path;
            currentLocation.path = removeSemicolon(currentLocation.path);
            inLocationBlock = true;
        } else if (directive == "}") {
            if (inLocationBlock) {
                locations.push_back(currentLocation);
                inLocationBlock = false;
            }
        } else if (directive == "listen") {
            iss >> port;
        } else if (directive == "server_name") {
            iss >> serverName;
            serverName = removeSemicolon(serverName);
        } else if (directive == "host") {
            iss >> host;
            host = removeSemicolon(host);
        } else if (directive == "root") {
            std::string rootPath;
            iss >> rootPath;
            rootPath = removeSemicolon(rootPath);
            if (inLocationBlock) {
                currentLocation.root = rootPath;
            } else {
                root = rootPath;
            }
        } else if (directive == "index") {
            std::string indexFile;
            iss >> indexFile;
            indexFile = removeSemicolon(indexFile);
            if (inLocationBlock) {
                currentLocation.index = indexFile;
            } else {
                index = indexFile;
            }
        } else if (directive == "client_max_body_size") {
            iss >> clientMaxBodySize;
        } else if (directive == "allow_methods") {
            if (inLocationBlock) {
                std::string method;
                while (iss >> method) {
                    method = removeSemicolon(method);
                    currentLocation.allowMethods.push_back(method);
                }
            }
        } else if (directive == "autoindex") {
            if (inLocationBlock) {
                std::string value;
                iss >> value;
                currentLocation.autoindex = (removeSemicolon(value) == "on");
            }
        } else if (directive == "cgi_path") {
            if (inLocationBlock) {
                std::string path;
                iss >> path;
                currentLocation.cgiPath.push_back(removeSemicolon(path));
            }
        } else if (directive == "cgi_ext") {
            if (inLocationBlock) {
                std::string ext;
                iss >> ext;
                currentLocation.cgiExt.push_back(removeSemicolon(ext));
            }
        } else if (directive == "upload_dir") {
            if (inLocationBlock) {
                iss >> currentLocation.uploadDir;
                currentLocation.uploadDir = removeSemicolon(currentLocation.uploadDir);
            }
        } else if (directive == "error_page") {
            int code;
            std::string page;
            iss >> code >> page;
            errorPages[code] = removeSemicolon(page);
        }
    }
    
    if (inLocationBlock) {
        locations.push_back(currentLocation);
    }
    
    file.close();
    return true;
}

std::string Config::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string Config::removeSemicolon(const std::string& str) {
    if (!str.empty() && str[str.length() - 1] == ';') {
        return str.substr(0, str.length() - 1);
    }
    return str;
}

const Location* Config::findLocation(const std::string& path) const {
    const Location* bestMatch = NULL;
    size_t bestMatchLength = 0;
    
    for (size_t i = 0; i < locations.size(); ++i) {
        const std::string& locPath = locations[i].path;
        if (path.find(locPath) == 0 && locPath.length() > bestMatchLength) {
            bestMatch = &locations[i];
            bestMatchLength = locPath.length();
        }
    }
    
    return bestMatch;
}

bool Config::isMethodAllowed(const std::string& method, const Location* loc) const {
    if (!loc || loc->allowMethods.empty()) {
        return true; // Default: allow all methods
    }
    
    for (size_t i = 0; i < loc->allowMethods.size(); ++i) {
        if (loc->allowMethods[i] == method) {
            return true;
        }
    }
    return false;
}
