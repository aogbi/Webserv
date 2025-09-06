/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi_handler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:58:47 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include "request.hpp"
#include "config.hpp"

class CgiHandler {
private:
    const Config& config;
    
    /**
     * @brief Set up CGI environment variables
     * @param req The HTTP request
     * @param scriptPath Path to the CGI script
     * @return Vector of environment variables
     */
    std::vector<std::string> setupCgiEnvironment(const Request& req, const std::string& scriptPath);
    
    /**
     * @brief Convert vector of strings to char* array for execve
     * @param env Vector of environment variables
     * @return char* array (must be freed after use)
     */
    char** vectorToCharArray(const std::vector<std::string>& env);
    
    /**
     * @brief Free char* array
     * @param arr The array to free
     */
    void freeCharArray(char** arr);

public:
    CgiHandler(const Config& config);
    
    /**
     * @brief Check if a request should be handled by CGI
     * @param path The request path
     * @param location The matched location block
     * @return true if CGI should handle this request
     */
    bool isCgiRequest(const std::string& path, const Location* location);
    
    /**
     * @brief Get the CGI interpreter for a file extension
     * @param extension File extension (e.g., ".py", ".php")
     * @param location The matched location block
     * @return Path to the interpreter, or empty string if not found
     */
    std::string getCgiInterpreter(const std::string& extension, const Location* location);
    
    /**
     * @brief Execute CGI script and return the output
     * @param scriptPath Path to the CGI script
     * @param req The HTTP request
     * @param location The matched location block
     * @return CGI output as HTTP response
     */
    std::string executeCgi(const std::string& scriptPath, const Request& req, const Location* location);
};

#endif // CGI_HANDLER_HPP
