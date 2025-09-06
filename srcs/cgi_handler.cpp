/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi_handler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 12:26:14 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cgi_handler.hpp"
#include "response.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstring>
#include <sstream>

CgiHandler::CgiHandler(const Config& config) : config(config) {
}

bool CgiHandler::isCgiRequest(const std::string& path, const Location* location) {
    if (!location) {
        return false;
    }
    
    // Find file extension
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string extension = path.substr(pos);
    
    // Check if extension is in CGI extensions list
    for (size_t i = 0; i < location->cgiExt.size(); ++i) {
        if (location->cgiExt[i] == extension) {
            return true;
        }
    }
    
    return false;
}

std::string CgiHandler::getCgiInterpreter(const std::string& extension, const Location* location) {
    if (!location) {
        return "";
    }
    
    // Find matching extension and return corresponding interpreter
    for (size_t i = 0; i < location->cgiExt.size() && i < location->cgiPath.size(); ++i) {
        if (location->cgiExt[i] == extension) {
            return location->cgiPath[i];
        }
    }
    
    return "";
}

std::string CgiHandler::executeCgi(const std::string& scriptPath, const Request& req, const Location* location) {
    Response response;
    
    // Get file extension
    size_t pos = scriptPath.find_last_of('.');
    if (pos == std::string::npos) {
        response.setStatus(500, "Internal Server Error");
        response.setContentType("text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>No file extension</p></body></html>");
        return response.toString();
    }
    
    std::string extension = scriptPath.substr(pos);
    std::string interpreter = getCgiInterpreter(extension, location);
    
    if (interpreter.empty()) {
        response.setStatus(500, "Internal Server Error");
        response.setContentType("text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>No CGI interpreter found</p></body></html>");
        return response.toString();
    }
    
    // Create pipes for communication
    int pipeIn[2], pipeOut[2];
    if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1) {
        response.setStatus(500, "Internal Server Error");
        response.setContentType("text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Pipe creation failed</p></body></html>");
        return response.toString();
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        close(pipeIn[0]); close(pipeIn[1]);
        close(pipeOut[0]); close(pipeOut[1]);
        response.setStatus(500, "Internal Server Error");
        response.setContentType("text/html");
        response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Fork failed</p></body></html>");
        return response.toString();
    }
    
    if (pid == 0) {
        // Child process
        close(pipeIn[1]);  // Close write end of input pipe
        close(pipeOut[0]); // Close read end of output pipe
        
        // Redirect stdin and stdout
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        
        close(pipeIn[0]);
        close(pipeOut[1]);
        
        // Set up environment
        std::vector<std::string> envVars = setupCgiEnvironment(req, scriptPath);
        char** envp = vectorToCharArray(envVars);
        
        // Execute CGI script
        char* args[] = {const_cast<char*>(interpreter.c_str()), const_cast<char*>(scriptPath.c_str()), NULL};
        execve(interpreter.c_str(), args, envp);
        
        // If we reach here, execve failed
        freeCharArray(envp);
        _exit(1);
    } else {
        // Parent process
        close(pipeIn[0]);  // Close read end of input pipe
        close(pipeOut[1]); // Close write end of output pipe
        
        // Send request body to CGI script if it's a POST request
        if (req.getMethod() == "POST" && !req.getBody().empty()) {
            write(pipeIn[1], req.getBody().c_str(), req.getBody().length());
        }
        close(pipeIn[1]);
        
        // Read output from CGI script
        std::string output;
        char buffer[4096];
        ssize_t bytesRead;
        
        while ((bytesRead = read(pipeOut[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        close(pipeOut[0]);
        
        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);
        
        if (WEXITSTATUS(status) != 0) {
            response.setStatus(500, "Internal Server Error");
            response.setContentType("text/html");
            response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI script execution failed</p></body></html>");
            return response.toString();
        }
        
        // Parse CGI output (should include headers and body)
        size_t headerEnd = output.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = output.find("\n\n");
            if (headerEnd == std::string::npos) {
                // No headers found, treat entire output as body
                response.setStatus(200, "OK");
                response.setContentType("text/html");
                response.setBody(output);
                return response.toString();
            } else {
                headerEnd += 2;
            }
        } else {
            headerEnd += 4;
        }
        
        // Extract headers and body
        std::string headers = output.substr(0, headerEnd);
        std::string body = output.substr(headerEnd);
        
        // Create response with CGI output
        response.setStatus(200, "OK");
        response.setBody(body);
        
        // Parse and set headers from CGI output
        std::istringstream headerStream(headers);
        std::string line;
        while (std::getline(headerStream, line) && !line.empty()) {
            if (line[line.length() - 1] == '\r') {
                line.erase(line.length() - 1);
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                // Trim whitespace
                while (!value.empty() && value[0] == ' ') value.erase(0, 1);
                while (!value.empty() && value[value.length() - 1] == ' ') value.erase(value.length() - 1);
                
                response.setHeader(key, value);
            }
        }
        
        return response.toString();
    }
}

std::vector<std::string> CgiHandler::setupCgiEnvironment(const Request& req, const std::string& scriptPath) {
    std::vector<std::string> env;
    
    // Basic CGI environment variables
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_SOFTWARE=Webserv/1.0");
    env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    env.push_back("REQUEST_METHOD=" + req.getMethod());
    env.push_back("REQUEST_URI=" + req.getPath());
    env.push_back("SCRIPT_NAME=" + scriptPath);
    env.push_back("QUERY_STRING="); // TODO: Extract from URL
    
    // Server-specific environment variables from config
    env.push_back("SERVER_NAME=" + config.getServerName());
    
    std::stringstream portStr;
    portStr << config.getPort();
    env.push_back("SERVER_PORT=" + portStr.str());
    
    // Content-related variables
    if (req.hasHeader("content-length")) {
        env.push_back("CONTENT_LENGTH=" + req.getHeader("content-length"));
    }
    if (req.hasHeader("content-type")) {
        env.push_back("CONTENT_TYPE=" + req.getHeader("content-type"));
    }
    
    // HTTP headers as environment variables
    const std::map<std::string, std::string>& headers = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string key = "HTTP_" + it->first;
        // Convert to uppercase and replace - with _
        for (size_t i = 0; i < key.length(); ++i) {
            if (key[i] == '-') {
                key[i] = '_';
            } else {
                key[i] = std::toupper(key[i]);
            }
        }
        env.push_back(key + "=" + it->second);
    }
    
    // Copy existing environment
    for (char** envp = environ; *envp != NULL; ++envp) {
        env.push_back(std::string(*envp));
    }
    
    return env;
}

char** CgiHandler::vectorToCharArray(const std::vector<std::string>& env) {
    char** arr = new char*[env.size() + 1];
    for (size_t i = 0; i < env.size(); ++i) {
        arr[i] = new char[env[i].length() + 1];
        std::strcpy(arr[i], env[i].c_str());
    }
    arr[env.size()] = NULL;
    return arr;
}

void CgiHandler::freeCharArray(char** arr) {
    for (int i = 0; arr[i] != NULL; ++i) {
        delete[] arr[i];
    }
    delete[] arr;
}
