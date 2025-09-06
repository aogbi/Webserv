/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_handler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 11:58:02 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_HANDLER_HPP
#define HTTP_HANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include "request.hpp"
#include "response.hpp"
#include "config.hpp"

class HttpHandler {
private:
    const Config& config;
    
    // File serving methods
    std::string serveFile(const std::string& requestPath, const Request& req);
    std::string generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);
    std::string getMimeType(const std::string& filename);
    
    // File upload methods
    std::string handleFileUpload(const Request& req, const Location* location);
    bool saveUploadedFile(const std::string& content, const std::string& filename, const std::string& uploadDir);
    
    // Error handling
    std::string generateErrorPage(int statusCode, const std::string& message);

public:
    HttpHandler(const Config& config);
    
    /**
     * @brief Handle HTTP request and generate response
     * @param req The HTTP request to handle
     * @return HTTP response as string
     */
    std::string handleRequest(const Request& req);
    
    /**
     * @brief Handle GET requests
     * @param req The HTTP request
     * @param location The matched location block
     * @return HTTP response as string
     */
    std::string handleGetRequest(const Request& req, const Location* location);
    
    /**
     * @brief Handle POST requests
     * @param req The HTTP request
     * @param location The matched location block
     * @return HTTP response as string
     */
    std::string handlePostRequest(const Request& req, const Location* location);
    
    /**
     * @brief Handle DELETE requests
     * @param req The HTTP request
     * @param location The matched location block
     * @return HTTP response as string
     */
    std::string handleDeleteRequest(const Request& req, const Location* location);
};

#endif // HTTP_HANDLER_HPP
