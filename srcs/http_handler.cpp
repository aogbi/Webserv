/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   http_handler.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 12:16:17 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "http_handler.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cerrno>

HttpHandler::HttpHandler(const Config& config) : config(config) {
}

std::string HttpHandler::handleRequest(const Request& req) {
    Response response;
    
    // Find matching location
    const Location* location = config.findLocation(req.getPath());
    
    // Check if method is allowed
    if (!config.isMethodAllowed(req.getMethod(), location)) {
        response.setStatus(405, "Method Not Allowed");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(405, "Method Not Allowed"));
        return response.toString();
    }
    
    // Handle different HTTP methods
    std::string method = req.getMethod();
    if (method == "GET" || method == "HEAD") {
        return handleGetRequest(req, location);
    } else if (method == "POST") {
        return handlePostRequest(req, location);
    } else if (method == "DELETE") {
        return handleDeleteRequest(req, location);
    } else {
        response.setStatus(501, "Not Implemented");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(501, "Method Not Implemented"));
        return response.toString();
    }
}

std::string HttpHandler::handleGetRequest(const Request& req, const Location* location) {
    std::string requestPath = req.getPath();
    
    // Handle redirect
    if (location && !location->redirect.empty()) {
        Response response;
        response.setRedirect(location->redirect, 301);
        return response.toString();
    }
    
    return serveFile(requestPath, req);
}

std::string HttpHandler::handlePostRequest(const Request& req, const Location* location) {
    Response response;
    
    // Check if this is a file upload request
    std::string contentType = req.getHeader("content-type");
    if (contentType.find("multipart/form-data") != std::string::npos && location && !location->uploadDir.empty()) {
        return handleFileUpload(req, location);
    } else {
        // Regular POST request
        response.setStatus(200, "OK");
        response.setContentType("text/html");
        std::ostringstream bodyStr;
        bodyStr << "<html><body><h1>POST request received</h1><p>Body length: " 
                << req.getBody().length() << " bytes</p></body></html>";
        response.setBody(bodyStr.str());
        return response.toString();
    }
}

std::string HttpHandler::handleDeleteRequest(const Request& req, const Location* location) {
    Response response;
    std::string requestPath = req.getPath();
    
    // Security check: Only allow deletion in uploads directory
    if (requestPath.find("/uploads/") != 0) {
        response.setStatus(403, "Forbidden");
        response.setContentType("application/json");
        response.setBody("{\"error\": \"File deletion only allowed in uploads directory\"}");
        return response.toString();
    }
    
    // Extract filename from path
    std::string filename = requestPath.substr(9); // Remove "/uploads/" prefix
    if (filename.empty() || filename.find("..") != std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setContentType("application/json");
        response.setBody("{\"error\": \"Invalid filename\"}");
        return response.toString();
    }
    
    // Construct full file path
    std::string uploadDir;
    if (location && !location->uploadDir.empty()) {
        // Use location-specific upload directory
        if (location->uploadDir[0] == '/') {
            // Absolute path
            uploadDir = location->uploadDir;
        } else {
            // Relative to server root
            uploadDir = config.getRoot();
            if (uploadDir[uploadDir.length() - 1] != '/') {
                uploadDir += "/";
            }
            uploadDir += location->uploadDir;
        }
    } else {
        // Default: server root + uploads
        uploadDir = config.getRoot();
        if (uploadDir[uploadDir.length() - 1] != '/') {
            uploadDir += "/";
        }
        uploadDir += "uploads";
    }
    
    std::string fullPath = uploadDir + "/" + filename;
    
    // Check if file exists
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) != 0) {
        response.setStatus(404, "Not Found");
        response.setContentType("application/json");
        response.setBody("{\"error\": \"File not found\"}");
        return response.toString();
    }
    
    // Check if it's a regular file (not directory)
    if (!S_ISREG(fileStat.st_mode)) {
        response.setStatus(400, "Bad Request");
        response.setContentType("application/json");
        response.setBody("{\"error\": \"Cannot delete directories\"}");
        return response.toString();
    }
    
    // Attempt to delete the file
    if (unlink(fullPath.c_str()) == 0) {
        response.setStatus(200, "OK");
        response.setContentType("application/json");
        response.setBody("{\"message\": \"File deleted successfully\", \"filename\": \"" + filename + "\"}");
        std::cout << "File deleted: " << fullPath << std::endl;
    } else {
        response.setStatus(500, "Internal Server Error");
        response.setContentType("application/json");
        response.setBody("{\"error\": \"Failed to delete file\"}");
        std::cerr << "Failed to delete file: " << fullPath << " - " << strerror(errno) << std::endl;
    }
    
    return response.toString();
}

std::string HttpHandler::serveFile(const std::string& requestPath, const Request& req) {
    (void)req; // Suppress unused parameter warning
    Response response;
    
    // Construct full file path
    std::string fullPath = config.getRoot() + requestPath;
    
    // Remove leading slash if present to avoid double slash
    if (!fullPath.empty() && fullPath[0] == '/' && fullPath[1] == '/') {
        fullPath = fullPath.substr(1);
    }
    
    struct stat fileStat;
    if (stat(fullPath.c_str(), &fileStat) == -1) {
        response.setStatus(404, "Not Found");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(404, "File Not Found"));
        return response.toString();
    }
    
    // Check if it's a directory
    if (S_ISDIR(fileStat.st_mode)) {
        // Try to serve index file
        std::string indexPath = fullPath;
        if (indexPath[indexPath.length() - 1] != '/') {
            indexPath += "/";
        }
        indexPath += config.getIndex();
        
        if (stat(indexPath.c_str(), &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
            fullPath = indexPath;
        } else {
            // Generate directory listing
            const Location* location = config.findLocation(requestPath);
            if (location && location->autoindex) {
                response.setStatus(200, "OK");
                response.setContentType("text/html");
                if (req.getMethod() != "HEAD") {
                    response.setBody(generateDirectoryListing(fullPath, requestPath));
                }
                return response.toString();
            } else {
                response.setStatus(403, "Forbidden");
                response.setContentType("text/html");
                if (req.getMethod() != "HEAD") {
                    response.setBody(generateErrorPage(403, "Directory listing forbidden"));
                }
                return response.toString();
            }
        }
    }
    
    // Serve regular file
    std::ifstream file(fullPath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        response.setStatus(403, "Forbidden");
        response.setContentType("text/html");
        if (req.getMethod() != "HEAD") {
            response.setBody(generateErrorPage(403, "Access Forbidden"));
        }
        return response.toString();
    }
    
    std::string content;
    if (req.getMethod() != "HEAD") {
        content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    } else {
        // For HEAD requests, we need to get the file size for Content-Length header
        // Use the stat information we already have
        std::ostringstream sizeStr;
        sizeStr << fileStat.st_size;
        response.setHeader("Content-Length", sizeStr.str());
    }
    file.close();
    
    response.setStatus(200, "OK");
    response.setContentType(getMimeType(fullPath));
    if (req.getMethod() != "HEAD") {
        response.setBody(content);
    }
    return response.toString();
}

std::string HttpHandler::generateDirectoryListing(const std::string& dirPath, const std::string& requestPath) {
    std::ostringstream html;
    html << "<html><head><title>Directory Listing</title></head><body>";
    html << "<h1>Directory Listing for " << requestPath << "</h1><ul>";
    
    DIR* dir = opendir(dirPath.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            if (name != ".") {
                std::string link = requestPath;
                if (link[link.length() - 1] != '/') {
                    link += "/";
                }
                link += name;
                html << "<li><a href=\"" << link << "\">" << name << "</a></li>";
            }
        }
        closedir(dir);
    }
    
    html << "</ul></body></html>";
    return html.str();
}

std::string HttpHandler::handleFileUpload(const Request& req, const Location* location) {
    Response response;
    
    std::string contentType = req.getHeader("content-type");
    std::string body = req.getBody();
    
    // Check file size limit (default: 10MB)
    const size_t MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    if (body.length() > MAX_FILE_SIZE) {
        response.setStatus(413, "Payload Too Large");
        response.setContentType("text/html");
        std::ostringstream errorBody;
        errorBody << "<html><head><title>File Too Large</title></head><body>";
        errorBody << "<h1>❌ File Too Large</h1>";
        errorBody << "<p>The uploaded file exceeds the maximum size limit of " << (MAX_FILE_SIZE / (1024 * 1024)) << " MB.</p>";
        errorBody << "<p><strong>Your file size:</strong> " << (body.length() / 1024) << " KB</p>";
        errorBody << "<div style='margin-top: 20px;'>";
        errorBody << "<a href='/upload.html' style='text-decoration: none; background: #007bff; color: white; padding: 10px 20px; border-radius: 5px;'>🔄 Try Again</a>";
        errorBody << "</div></body></html>";
        response.setBody(errorBody.str());
        return response.toString();
    }
    
    // Extract boundary from Content-Type
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(400, "No boundary found in multipart data"));
        return response.toString();
    }
    
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);
    
    // Find file data in multipart body
    size_t fileStart = body.find("Content-Disposition: form-data");
    if (fileStart == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(400, "No form data found"));
        return response.toString();
    }
    
    // Extract filename
    size_t filenamePos = body.find("filename=\"", fileStart);
    if (filenamePos == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(400, "No filename found"));
        return response.toString();
    }
    
    filenamePos += 10; // Skip 'filename="'
    size_t filenameEnd = body.find("\"", filenamePos);
    std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);
    
    // Validate filename - prevent directory traversal and illegal characters
    if (filename.empty() || filename.find("..") != std::string::npos || 
        filename.find("/") != std::string::npos || filename.find("\\") != std::string::npos ||
        filename[0] == '.' || filename.length() > 255) {
        response.setStatus(400, "Bad Request");
        response.setContentType("text/html");
        std::ostringstream errorBody;
        errorBody << "<html><head><title>Invalid Filename</title></head><body>";
        errorBody << "<h1>❌ Invalid Filename</h1>";
        errorBody << "<p>The filename contains invalid characters or is not allowed.</p>";
        errorBody << "<p><strong>Rules:</strong></p><ul>";
        errorBody << "<li>No directory traversal (../)</li>";
        errorBody << "<li>No path separators (/ or \\)</li>";
        errorBody << "<li>Cannot start with a dot</li>";
        errorBody << "<li>Maximum length: 255 characters</li></ul>";
        errorBody << "<div style='margin-top: 20px;'>";
        errorBody << "<a href='/upload.html' style='text-decoration: none; background: #007bff; color: white; padding: 10px 20px; border-radius: 5px;'>🔄 Try Again</a>";
        errorBody << "</div></body></html>";
        response.setBody(errorBody.str());
        return response.toString();
    }
    
    // Find file content start (after headers)
    size_t contentStart = body.find("\r\n\r\n", fileStart);
    if (contentStart == std::string::npos) {
        contentStart = body.find("\n\n", fileStart);
        if (contentStart == std::string::npos) {
            response.setStatus(400, "Bad Request");
            response.setContentType("text/html");
            response.setBody(generateErrorPage(400, "Invalid multipart format"));
            return response.toString();
        }
        contentStart += 2;
    } else {
        contentStart += 4;
    }
    
    // Find file content end
    size_t contentEnd = body.find(boundary, contentStart);
    if (contentEnd == std::string::npos) {
        response.setStatus(400, "Bad Request");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(400, "Invalid multipart format"));
        return response.toString();
    }
    
    // Remove trailing CRLF before boundary
    while (contentEnd > contentStart && (body[contentEnd - 1] == '\r' || body[contentEnd - 1] == '\n')) {
        contentEnd--;
    }
    
    std::string fileContent = body.substr(contentStart, contentEnd - contentStart);
    
    // Construct upload directory path
    std::string uploadDir;
    if (location && !location->uploadDir.empty()) {
        if (location->uploadDir[0] == '/') {
            uploadDir = location->uploadDir;
        } else {
            uploadDir = config.getRoot();
            if (uploadDir[uploadDir.length() - 1] != '/') {
                uploadDir += "/";
            }
            uploadDir += location->uploadDir;
        }
    } else {
        uploadDir = config.getRoot();
        if (uploadDir[uploadDir.length() - 1] != '/') {
            uploadDir += "/";
        }
        uploadDir += "uploads";
    }
    
    // Save the file
    if (saveUploadedFile(fileContent, filename, uploadDir)) {
        response.setStatus(200, "OK");
        response.setContentType("text/html");
        std::ostringstream responseBody;
        responseBody << "<html><head><title>Upload Success</title></head><body>";
        responseBody << "<h1>✅ File Upload Successful</h1>";
        responseBody << "<p>File <strong>" << filename << "</strong> has been uploaded successfully.</p>";
        responseBody << "<p><strong>File size:</strong> " << fileContent.length() << " bytes</p>";
        responseBody << "<p><strong>Upload directory:</strong> " << uploadDir << "</p>";
        responseBody << "<div style='margin-top: 20px;'>";
        responseBody << "<a href='/' style='text-decoration: none; background: #007bff; color: white; padding: 10px 20px; border-radius: 5px; margin-right: 10px;'>🏠 Home</a>";
        responseBody << "<a href='/uploads.html' style='text-decoration: none; background: #28a745; color: white; padding: 10px 20px; border-radius: 5px; margin-right: 10px;'>📁 View Uploads</a>";
        responseBody << "<a href='/upload.html' style='text-decoration: none; background: #17a2b8; color: white; padding: 10px 20px; border-radius: 5px;'>📤 Upload More</a>";
        responseBody << "</div>";
        responseBody << "</body></html>";
        response.setBody(responseBody.str());
    } else {
        response.setStatus(500, "Internal Server Error");
        response.setContentType("text/html");
        response.setBody(generateErrorPage(500, "Failed to save uploaded file"));
    }
    
    return response.toString();
}

bool HttpHandler::saveUploadedFile(const std::string& content, const std::string& filename, const std::string& uploadDir) {
    // Create upload directory if it doesn't exist
    struct stat st;
    if (stat(uploadDir.c_str(), &st) != 0) {
        if (mkdir(uploadDir.c_str(), 0755) != 0) {
            std::cerr << "Failed to create upload directory: " << uploadDir << std::endl;
            return false;
        }
    }
    
    // Create full file path
    std::string filePath = uploadDir + "/" + filename;
    
    // Write file
    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return false;
    }
    
    file.write(content.c_str(), content.length());
    file.close();
    
    if (file.fail()) {
        std::cerr << "Failed to write file: " << filePath << std::endl;
        return false;
    }
    
    std::cout << "File uploaded successfully: " << filePath << " (" << content.length() << " bytes)" << std::endl;
    return true;
}

std::string HttpHandler::getMimeType(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string extension = filename.substr(pos);
    
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".js") return "application/javascript";
    if (extension == ".json") return "application/json";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".png") return "image/png";
    if (extension == ".gif") return "image/gif";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".txt") return "text/plain";
    if (extension == ".xml") return "application/xml";
    if (extension == ".pdf") return "application/pdf";
    
    return "application/octet-stream";
}

std::string HttpHandler::generateErrorPage(int statusCode, const std::string& message) {
    std::ostringstream html;
    html << "<html><head><title>Error " << statusCode << "</title></head><body>";
    html << "<h1>Error " << statusCode << "</h1>";
    html << "<p>" << message << "</p>";
    html << "<hr><p>Webserv/1.0</p></body></html>";
    return html.str();
}
