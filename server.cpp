/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:46:51 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:48:46 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "request.hpp"
#include "response.hpp"


Server::Server(std::string file) : configfile(file), port(8080), server_fd(-1), serverName("localhost"), host("127.0.0.1"), root("./"), index("index.html"), clientMaxBodySize(1000000) {
	parseConfig();
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	std::stringstream ss;
	ss << port;
	std::string portStr = ss.str();
	if (getaddrinfo(NULL, portStr.c_str(), &hints, &res) != 0) {
		std::cerr << "getaddrinfo failed: " << gai_strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	address = *(struct sockaddr_in *)res->ai_addr;
	freeaddrinfo(res);
}

Server::~Server() {
	if (server_fd != -1) {
		close(server_fd);
		std::cout << "Server closed" << std::endl;
	}
}

bool Server::setup() {
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		std::cerr << "socket failed: " << strerror(errno) << std::endl;
		return false;
	}

	int opt = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		std::cerr << "bind failed: " << strerror(errno) << std::endl;
		return false;
	}

	if (listen(server_fd, SOMAXCONN) < 0) {
		std::cerr << "listen failed: " << strerror(errno) << std::endl;
		return false;
	}

	std::cout << "Server listening on port " << port << std::endl;
	return true;
}

int Server::getSocket() const {
	return server_fd;
}
int Server::acceptClient() {
	addrlen = sizeof(address);
	int client_fd = accept(server_fd, (struct sockaddr*)&address, &addrlen);
	if (client_fd < 0) {
		std::cerr << "accept failed: " << strerror(errno) << std::endl;
		return -1;
	}
	std::cout << "New connection accepted" << std::endl;
	return client_fd;
}

void Server::run() {
	struct pollfd fds[200];
	fds[0].fd = server_fd;
	fds[0].events = POLLIN;
	int nfds = 1;

	while (true) {
		int activity = poll(fds, nfds, -1);
		if (activity < 0) {
			std::cerr << "poll error: " << strerror(errno) << std::endl;
			break;
		}

		if (fds[0].revents & POLLIN) {
			int client_fd = acceptClient();
			if (client_fd != -1 && nfds < 200) {
				fds[nfds].fd = client_fd;
				fds[nfds].events = POLLIN;
				nfds++;
			}
		}

		for (int i = 1; i < nfds; i++) {
			if (fds[i].revents & POLLIN) {
				char buffer[1024] = {0};
				int bytes_read = read(fds[i].fd, buffer, sizeof(buffer));
				if (bytes_read <= 0) {
					close(fds[i].fd);
					fds[i] = fds[nfds - 1];
					nfds--;
					i--;
				} else {
					Request req;
					if (req.parse(std::string(buffer, bytes_read))) {
						// Handle the request and generate response
						std::string responseStr = handleRequest(req);
						send(fds[i].fd, responseStr.c_str(), responseStr.length(), 0);
					} else {
						// Bad request
						Response resp;
						resp.setStatus(400, "Bad Request");
						resp.setBody("<html><body><h1>400 Bad Request</h1></body></html>");
						std::string responseStr = resp.toString();
						send(fds[i].fd, responseStr.c_str(), responseStr.length(), 0);
					}
				}
			}
		}
	}
}

int Server::parseConfig() {
	std::ifstream file(configfile.c_str());
	if (!file.is_open()) {
		std::cerr << "Failed to open config file: " << configfile << std::endl;
		return 1;
	}
	
	std::string line;
	bool inServerBlock = false;
	bool inLocationBlock = false;
	Location currentLocation;
	
	while (std::getline(file, line)) {
		// Remove comments and trim whitespace
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos) {
			line = line.substr(0, commentPos);
		}
		
		std::istringstream iss(line);
		std::string key;
		if (!(iss >> key))
			continue;
			
		if (key == "server" && !inServerBlock) {
			inServerBlock = true;
			continue;
		}
		
		if (key == "}" && inLocationBlock) {
			locations.push_back(currentLocation);
			currentLocation = Location();
			inLocationBlock = false;
			continue;
		}
		
		if (key == "}" && inServerBlock) {
			inServerBlock = false;
			continue;
		}
		
		if (!inServerBlock)
			continue;
			
		if (key == "location") {
			std::string path;
			if (iss >> path) {
				currentLocation.path = path;
				currentLocation.autoindex = false;
				currentLocation.index = "";
				currentLocation.root = "";
				currentLocation.redirect = "";
				inLocationBlock = true;
			}
			continue;
		}
		
		if (inLocationBlock) {
			if (key == "allow_methods") {
				std::string method;
				currentLocation.allowMethods.clear();
				while (iss >> method) {
					// Remove semicolon if present
					if (!method.empty() && method[method.length() - 1] == ';') {
						method = method.substr(0, method.length() - 1);
					}
					currentLocation.allowMethods.push_back(method);
				}
			} else if (key == "autoindex") {
				std::string value;
				if (iss >> value) {
					currentLocation.autoindex = (value == "on");
				}
			} else if (key == "index") {
				std::string value;
				if (iss >> value) {
					currentLocation.index = value;
				}
			} else if (key == "root") {
				std::string value;
				if (iss >> value) {
					// Remove semicolon if present
					if (!value.empty() && value[value.length() - 1] == ';') {
						value = value.substr(0, value.length() - 1);
					}
					currentLocation.root = value;
				}
			} else if (key == "return") {
				std::string value;
				if (iss >> value) {
					currentLocation.redirect = value;
				}
			} else if (key == "cgi_path") {
				std::string path;
				currentLocation.cgiPath.clear();
				while (iss >> path) {
					currentLocation.cgiPath.push_back(path);
				}
			} else if (key == "cgi_ext") {
				std::string ext;
				currentLocation.cgiExt.clear();
				while (iss >> ext) {
					currentLocation.cgiExt.push_back(ext);
				}
			}
		} else {
			if (key == "listen") {
				std::string value;
				if (iss >> value) {
					port = static_cast<uint16_t>(std::atoi(value.c_str()));
				}
			} else if (key == "server_name") {
				if (iss >> serverName) {
					// server_name parsed
				}
			} else if (key == "host") {
				if (iss >> host) {
					// host parsed
				}
			} else if (key == "root") {
				if (iss >> root) {
					// Remove semicolon if present
					if (!root.empty() && root[root.length() - 1] == ';') {
						root = root.substr(0, root.length() - 1);
					}
				}
			} else if (key == "index") {
				if (iss >> index) {
					// Remove semicolon if present
					if (!index.empty() && index[index.length() - 1] == ';') {
						index = index.substr(0, index.length() - 1);
					}
				}
			} else if (key == "error_page") {
				int code;
				std::string page;
				if (iss >> code >> page) {
					errorPages[code] = page;
				}
			} else if (key == "client_max_body_size") {
				std::string value;
				if (iss >> value) {
					clientMaxBodySize = std::atoi(value.c_str());
				}
			}
		}
	}
	
	file.close();
	return 0;
}

std::string Server::handleRequest(const Request& req) {
	Response response;
	
	// Find matching location
	Location* loc = findLocation(req.getPath());
	
	// Check if method is allowed
	if (loc && !isMethodAllowed(req.getMethod(), loc)) {
		response.setStatus(405, "Method Not Allowed");
		response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
		return response.toString();
	}
	
	// Handle redirects
	if (loc && !loc->redirect.empty()) {
		response.setStatus(301, "Moved Permanently");
		response.setHeader("Location", loc->redirect);
		response.setBody("<html><body><h1>301 Moved Permanently</h1></body></html>");
		return response.toString();
	}
	
	// Handle different HTTP methods
	if (req.getMethod() == "GET" || req.getMethod() == "HEAD") {
		std::string result = serveFile(req.getPath(), req);
		if (req.getMethod() == "HEAD") {
			// For HEAD requests, remove the body but keep headers
			size_t bodyPos = result.find("\r\n\r\n");
			if (bodyPos != std::string::npos) {
				result = result.substr(0, bodyPos + 4);
			}
		}
		return result;
	} else if (req.getMethod() == "POST") {
		// For now, just return a simple response for POST
		response.setStatus(200, "OK");
		response.setBody("<html><body><h1>POST received</h1></body></html>");
		return response.toString();
	} else if (req.getMethod() == "DELETE") {
		// For now, just return a simple response for DELETE
		response.setStatus(200, "OK");
		response.setBody("<html><body><h1>DELETE received</h1></body></html>");
		return response.toString();
	} else {
		response.setStatus(501, "Not Implemented");
		response.setBody("<html><body><h1>501 Not Implemented</h1></body></html>");
		return response.toString();
	}
}

std::string Server::serveFile(const std::string& requestPath, const Request& req) {
	Response response;
	
	// Find matching location first
	Location* loc = findLocation(requestPath);
	
	// Determine the actual file path using location-specific root if available
	std::string fileRoot = (loc && !loc->root.empty()) ? loc->root : root;
	std::string filePath = fileRoot;
	if (filePath[filePath.length() - 1] != '/') {
		filePath += "/";
	}
	
	std::string cleanPath = requestPath;
	if (cleanPath[0] == '/') {
		cleanPath = cleanPath.substr(1);
	}
	filePath += cleanPath;
	
	// Check for directory traversal
	if (filePath.find("..") != std::string::npos) {
		response.setStatus(403, "Forbidden");
		response.setBody("<html><body><h1>403 Forbidden</h1></body></html>");
		return response.toString();
	}
	
	// Check if this is a CGI request
	if (isCgiRequest(filePath, loc)) {
		std::cout << "DEBUG: CGI path: " << filePath << std::endl;
		return executeCgi(filePath, req, loc);
	}
	
	struct stat statbuf;
	if (stat(filePath.c_str(), &statbuf) != 0) {
		// File not found
		response.setStatus(404, "Not Found");
		std::string errorBody = "<html><body><h1>404 Not Found</h1><p>The requested file was not found.</p></body></html>";
		
		// Check for custom error page
		if (errorPages.find(404) != errorPages.end()) {
			std::string errorPagePath = root + "/" + errorPages[404];
			std::ifstream errorFile(errorPagePath.c_str(), std::ios::binary);
			if (errorFile.is_open()) {
				std::ostringstream contents;
				contents << errorFile.rdbuf();
				errorBody = contents.str();
				errorFile.close();
			}
		}
		
		response.setBody(errorBody);
		return response.toString();
	}
	
	if (S_ISDIR(statbuf.st_mode)) {
		// It's a directory
		Location* loc = findLocation(requestPath);
		
		// Try to serve index file
		std::string indexFile = (loc && !loc->index.empty()) ? loc->index : index;
		std::string indexPath = filePath;
		if (indexPath[indexPath.length() - 1] != '/') {
			indexPath += "/";
		}
		indexPath += indexFile;
		
		if (stat(indexPath.c_str(), &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
			// Serve the index file
			std::ifstream file(indexPath.c_str(), std::ios::binary);
			if (file.is_open()) {
				std::ostringstream contents;
				contents << file.rdbuf();
				file.close();
				
				response.setStatus(200, "OK");
				response.setHeader("Content-Type", getMimeType(indexPath));
				response.setBody(contents.str());
				return response.toString();
			}
		}
		
		// Check if autoindex is enabled
		if (loc && loc->autoindex) {
			std::string listing = generateDirectoryListing(filePath, requestPath);
			response.setStatus(200, "OK");
			response.setHeader("Content-Type", "text/html");
			response.setBody(listing);
			return response.toString();
		} else {
			response.setStatus(403, "Forbidden");
			response.setBody("<html><body><h1>403 Forbidden</h1><p>Directory listing is not allowed.</p></body></html>");
			return response.toString();
		}
	} else {
		// It's a regular file
		std::ifstream file(filePath.c_str(), std::ios::binary);
		if (!file.is_open()) {
			response.setStatus(403, "Forbidden");
			response.setBody("<html><body><h1>403 Forbidden</h1></body></html>");
			return response.toString();
		}
		
		std::ostringstream contents;
		contents << file.rdbuf();
		file.close();
		
		response.setStatus(200, "OK");
		response.setHeader("Content-Type", getMimeType(filePath));
		response.setBody(contents.str());
		return response.toString();
	}
}

std::string Server::generateDirectoryListing(const std::string& dirPath, const std::string& requestPath) {
	std::ostringstream html;
	html << "<html><head><title>Directory listing for " << requestPath << "</title></head><body>";
	html << "<h1>Directory listing for " << requestPath << "</h1><hr><ul>";
	
	DIR* dir = opendir(dirPath.c_str());
	if (dir != NULL) {
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			std::string name = entry->d_name;
			if (name == "." || name == "..")
				continue;
				
			std::string fullPath = dirPath + "/" + name;
			struct stat statbuf;
			if (stat(fullPath.c_str(), &statbuf) == 0) {
				std::string linkPath = requestPath;
				if (linkPath[linkPath.length() - 1] != '/') {
					linkPath += "/";
				}
				linkPath += name;
				
				if (S_ISDIR(statbuf.st_mode)) {
					html << "<li><a href=\"" << linkPath << "/\">" << name << "/</a></li>";
				} else {
					html << "<li><a href=\"" << linkPath << "\">" << name << "</a></li>";
				}
			}
		}
		closedir(dir);
	}
	
	html << "</ul><hr></body></html>";
	return html.str();
}

std::string Server::getMimeType(const std::string& filename) {
	size_t dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos) {
		return "application/octet-stream";
	}
	
	std::string ext = filename.substr(dotPos);
	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css") return "text/css";
	if (ext == ".js") return "application/javascript";
	if (ext == ".json") return "application/json";
	if (ext == ".png") return "image/png";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".gif") return "image/gif";
	if (ext == ".txt") return "text/plain";
	if (ext == ".pdf") return "application/pdf";
	
	return "application/octet-stream";
}

Location* Server::findLocation(const std::string& path) {
	Location* bestMatch = NULL;
	size_t bestMatchLength = 0;
	
	for (std::vector<Location>::iterator it = locations.begin(); it != locations.end(); ++it) {
		if (path.find(it->path) == 0 && it->path.length() > bestMatchLength) {
			bestMatch = &(*it);
			bestMatchLength = it->path.length();
		}
	}
	
	return bestMatch;
}

bool Server::isMethodAllowed(const std::string& method, const Location* loc) {
	if (!loc || loc->allowMethods.empty()) {
		return true; // If no restrictions, allow all methods
	}
	
	for (std::vector<std::string>::const_iterator it = loc->allowMethods.begin(); it != loc->allowMethods.end(); ++it) {
		if (*it == method) {
			return true;
		}
	}
	
	return false;
}

bool Server::isCgiRequest(const std::string& path, const Location* loc) {
	if (!loc || loc->cgiExt.empty()) {
		return false;
	}
	
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos) {
		return false;
	}
	
	std::string extension = path.substr(dotPos);
	for (std::vector<std::string>::const_iterator it = loc->cgiExt.begin(); it != loc->cgiExt.end(); ++it) {
		if (*it == extension) {
			return true;
		}
	}
	
	return false;
}

std::string Server::getCgiInterpreter(const std::string& extension, const Location* loc) {
	if (!loc || loc->cgiPath.empty()) {
		return "";
	}
	
	// For simplicity, match extensions to interpreters in order
	// .py -> python, .sh -> bash, etc.
	if (extension == ".py" && loc->cgiPath.size() > 0) {
		return loc->cgiPath[0]; // First interpreter (should be python)
	} else if (extension == ".sh" && loc->cgiPath.size() > 1) {
		return loc->cgiPath[1]; // Second interpreter (should be bash)
	}
	
	return loc->cgiPath.empty() ? "" : loc->cgiPath[0];
}

std::string Server::executeCgi(const std::string& scriptPath, const Request& req, const Location* loc) {
	Response response;
	
	// Check if script exists
	struct stat statbuf;
	if (stat(scriptPath.c_str(), &statbuf) != 0) {
		response.setStatus(404, "Not Found");
		response.setBody("<html><body><h1>404 Not Found</h1><p>CGI script not found.</p></body></html>");
		return response.toString();
	}
	
	// Get the interpreter
	size_t dotPos = scriptPath.find_last_of('.');
	if (dotPos == std::string::npos) {
		response.setStatus(500, "Internal Server Error");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>No file extension.</p></body></html>");
		return response.toString();
	}
	
	std::string extension = scriptPath.substr(dotPos);
	std::string interpreter = getCgiInterpreter(extension, loc);
	
	if (interpreter.empty()) {
		response.setStatus(500, "Internal Server Error");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>No CGI interpreter found.</p></body></html>");
		return response.toString();
	}
	
	// Create pipes for communication
	int inputPipe[2];  // To send data to CGI
	int outputPipe[2]; // To receive data from CGI
	
	if (pipe(inputPipe) == -1 || pipe(outputPipe) == -1) {
		response.setStatus(500, "Internal Server Error");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to create pipes.</p></body></html>");
		return response.toString();
	}
	
	pid_t pid = fork();
	if (pid == -1) {
		// Fork failed
		close(inputPipe[0]);
		close(inputPipe[1]);
		close(outputPipe[0]);
		close(outputPipe[1]);
		response.setStatus(500, "Internal Server Error");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to fork process.</p></body></html>");
		return response.toString();
	}
	
	if (pid == 0) {
		// Child process - execute CGI
		close(inputPipe[1]);  // Close write end of input pipe
		close(outputPipe[0]); // Close read end of output pipe
		
		// Redirect stdin and stdout
		dup2(inputPipe[0], STDIN_FILENO);
		dup2(outputPipe[1], STDOUT_FILENO);
		
		// Set environment variables
		setenv("REQUEST_METHOD", req.getMethod().c_str(), 1);
		setenv("REQUEST_URI", req.getPath().c_str(), 1);
		setenv("SERVER_PROTOCOL", req.getVersion().c_str(), 1);
		setenv("CONTENT_TYPE", req.getHeader("Content-Type").c_str(), 1);
		setenv("CONTENT_LENGTH", req.getHeader("Content-Length").c_str(), 1);
		setenv("HTTP_HOST", req.getHeader("Host").c_str(), 1);
		setenv("HTTP_USER_AGENT", req.getHeader("User-Agent").c_str(), 1);
		
		// Execute the CGI script
		char* args[] = {(char*)interpreter.c_str(), (char*)scriptPath.c_str(), NULL};
		execve(interpreter.c_str(), args, environ);
		
		// If execve fails
		exit(1);
	} else {
		// Parent process
		close(inputPipe[0]);  // Close read end of input pipe
		close(outputPipe[1]); // Close write end of output pipe
		
		// Send request body to CGI if it's a POST request
		if (req.getMethod() == "POST" && !req.getBody().empty()) {
			write(inputPipe[1], req.getBody().c_str(), req.getBody().length());
		}
		close(inputPipe[1]); // Close write end to signal EOF
		
		// Read output from CGI
		std::string cgiOutput;
		char buffer[1024];
		ssize_t bytesRead;
		while ((bytesRead = read(outputPipe[0], buffer, sizeof(buffer))) > 0) {
			cgiOutput.append(buffer, bytesRead);
		}
		close(outputPipe[0]);
		
		// Wait for child process to finish
		int status;
		waitpid(pid, &status, 0);
		
		if (WEXITSTATUS(status) != 0) {
			response.setStatus(500, "Internal Server Error");
			response.setBody("<html><body><h1>500 Internal Server Error</h1><p>CGI script execution failed.</p></body></html>");
			return response.toString();
		}
		
		// Parse CGI output (headers + body)
		size_t headerEnd = cgiOutput.find("\r\n\r\n");
		if (headerEnd == std::string::npos) {
			headerEnd = cgiOutput.find("\n\n");
			if (headerEnd != std::string::npos) {
				headerEnd += 2;
			}
		} else {
			headerEnd += 4;
		}
		
		if (headerEnd != std::string::npos) {
			// Headers present
			std::string headers = cgiOutput.substr(0, headerEnd);
			std::string body = cgiOutput.substr(headerEnd);
			
			// Parse headers and build response
			response.setStatus(200, "OK");
			std::istringstream headerStream(headers);
			std::string line;
			while (std::getline(headerStream, line)) {
				if (line.empty() || line == "\r") break;
				size_t colonPos = line.find(':');
				if (colonPos != std::string::npos) {
					std::string key = line.substr(0, colonPos);
					std::string value = line.substr(colonPos + 1);
					// Trim whitespace
					while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
						value.erase(0, 1);
					}
					if (!value.empty() && value[value.length()-1] == '\r') {
						value.erase(value.length()-1);
					}
					response.setHeader(key, value);
				}
			}
			response.setBody(body);
		} else {
			// No headers, just body
			response.setStatus(200, "OK");
			response.setHeader("Content-Type", "text/html");
			response.setBody(cgiOutput);
		}
		
		return response.toString();
	}
}
