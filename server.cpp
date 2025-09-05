/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:46:51 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 17:16:56 by aogbi            ###   ########.fr       */
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
	const int MAX_CLIENTS = 200;
	const int POLL_TIMEOUT = 1000; // 1 second poll timeout
	
	std::vector<struct pollfd> fds(MAX_CLIENTS);
	std::map<int, ClientConnection> clients;
	
	fds[0].fd = server_fd;
	fds[0].events = POLLIN;
	int nfds = 1;

	std::cout << "🚀 Server running... (Press Ctrl+C to stop)" << std::endl;

	while (g_running) {
		// Handle client timeouts periodically
		handleClientTimeout(fds, clients, nfds);
		
		int activity = poll(&fds[0], nfds, POLL_TIMEOUT);
		
		if (activity < 0) {
			if (errno == EINTR && !g_running) {
				break; // Interrupted by signal, check g_running flag
			}
			std::cerr << "❌ Poll error: " << strerror(errno) << std::endl;
			break;
		}
		
		if (activity == 0) {
			continue; // Timeout, check g_running flag and continue
		}

		// Check for new connections
		if (fds[0].revents & POLLIN) {
			int client_fd = acceptClient();
			if (client_fd != -1) {
				if (nfds < MAX_CLIENTS) {
					fds[nfds].fd = client_fd;
					fds[nfds].events = POLLIN | POLLHUP | POLLERR;
					fds[nfds].revents = 0;
					clients[client_fd] = ClientConnection(client_fd);
					nfds++;
					std::cout << "📝 Client connected (fd: " << client_fd << "), total: " << (nfds - 1) << std::endl;
				} else {
					std::cerr << "⚠️  Max clients reached, rejecting connection" << std::endl;
					sendErrorResponse(client_fd, 503, "Service Unavailable");
					close(client_fd);
				}
			}
		}

		// Handle existing client connections
		for (int i = 1; i < nfds && g_running; i++) {
			int client_fd = fds[i].fd;
			
			// Handle client disconnection or errors
			if (fds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
				std::cout << "📤 Client disconnected (fd: " << client_fd << ")" << std::endl;
				removeClient(fds, clients, i, nfds);
				i--; // Adjust index after removal
				continue;
			}
			
			// Handle incoming data
			if (fds[i].revents & POLLIN) {
				char buffer[4096] = {0}; // Increased buffer size
				ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
				
				if (bytes_read <= 0) {
					if (bytes_read == 0) {
						std::cout << "📤 Client closed connection (fd: " << client_fd << ")" << std::endl;
					} else {
						std::cerr << "❌ Read error on client " << client_fd << ": " << strerror(errno) << std::endl;
					}
					removeClient(fds, clients, i, nfds);
					i--; // Adjust index after removal
					continue;
				}
				
				// Update client activity and append to buffer
				clients[client_fd].lastActivity = time(NULL);
				clients[client_fd].buffer.append(buffer, bytes_read);
				
				// Check if we have a complete request
				if (isRequestComplete(clients[client_fd].buffer)) {
					clients[client_fd].requestComplete = true;
					
					// Parse and handle the request
					Request req;
					if (req.parse(clients[client_fd].buffer)) {
						std::string responseStr = handleRequest(req);
						ssize_t sent = send(client_fd, responseStr.c_str(), responseStr.length(), MSG_NOSIGNAL);
						if (sent < 0) {
							std::cerr << "❌ Send error to client " << client_fd << ": " << strerror(errno) << std::endl;
						}
					} else {
						sendErrorResponse(client_fd, 400, "Bad Request");
					}
					
					// Close connection after response (HTTP/1.0 behavior)
					// TODO: Add Connection: keep-alive support for HTTP/1.1
					std::cout << "📤 Request completed, closing connection (fd: " << client_fd << ")" << std::endl;
					removeClient(fds, clients, i, nfds);
					i--; // Adjust index after removal
				}
			}
		}
	}
	
	// Cleanup: close all client connections
	std::cout << "🧹 Cleaning up connections..." << std::endl;
	for (std::map<int, ClientConnection>::iterator it = clients.begin(); it != clients.end(); ++it) {
		close(it->first);
	}
	clients.clear();
	
	std::cout << "✅ Server shutdown complete." << std::endl;
}

// Helper function to trim whitespace
std::string Server::trim(const std::string& str) {
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) return "";
	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

// Helper function to remove semicolon from end of string
std::string Server::removeSemicolon(const std::string& str) {
	std::string result = str;
	if (!result.empty() && result[result.length() - 1] == ';') {
		result = result.substr(0, result.length() - 1);
	}
	return trim(result);
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
	int lineNumber = 0;
	
	while (std::getline(file, line)) {
		lineNumber++;
		
		// Remove comments and trim whitespace
		size_t commentPos = line.find('#');
		if (commentPos != std::string::npos) {
			line = line.substr(0, commentPos);
		}
		line = trim(line);
		
		if (line.empty())
			continue;
		
		std::istringstream iss(line);
		std::string key;
		if (!(iss >> key))
			continue;
			
		if (key == "server") {
			if (inServerBlock) {
				std::cerr << "Error: Nested server blocks not allowed (line " << lineNumber << ")" << std::endl;
				file.close();
				return 1;
			}
			
			// Check for opening brace
			std::string brace;
			if (iss >> brace && brace == "{") {
				inServerBlock = true;
			} else if (line.find('{') != std::string::npos) {
				inServerBlock = true;
			} else {
				std::cerr << "Error: Expected '{' after server directive (line " << lineNumber << ")" << std::endl;
				file.close();
				return 1;
			}
			continue;
		}
		
		if (key == "}" && inLocationBlock) {
			locations.push_back(currentLocation);
			currentLocation = Location();
			inLocationBlock = false;
			continue;
		}
		
		if (key == "}" && inServerBlock && !inLocationBlock) {
			inServerBlock = false;
			continue;
		}
		
		if (!inServerBlock) {
			std::cerr << "Error: Directive outside server block (line " << lineNumber << "): " << key << std::endl;
			continue;
		}
			
		if (key == "location") {
			if (inLocationBlock) {
				std::cerr << "Error: Nested location blocks not allowed (line " << lineNumber << ")" << std::endl;
				file.close();
				return 1;
			}
			
			std::string path;
			if (iss >> path) {
				currentLocation.path = path;
				currentLocation.autoindex = false;
				currentLocation.index = "";
				currentLocation.root = "";
				currentLocation.redirect = "";
				currentLocation.uploadDir = "";
				currentLocation.allowMethods.clear();
				currentLocation.cgiPath.clear();
				currentLocation.cgiExt.clear();
				
				// Check for opening brace
				std::string brace;
				if (iss >> brace && brace == "{") {
					inLocationBlock = true;
				} else if (line.find('{') != std::string::npos) {
					inLocationBlock = true;
				} else {
					std::cerr << "Error: Expected '{' after location directive (line " << lineNumber << ")" << std::endl;
					file.close();
					return 1;
				}
			} else {
				std::cerr << "Error: Missing path for location directive (line " << lineNumber << ")" << std::endl;
				file.close();
				return 1;
			}
			continue;
		}
		
		if (inLocationBlock) {
			if (key == "allow_methods") {
				std::string method;
				currentLocation.allowMethods.clear();
				while (iss >> method) {
					method = removeSemicolon(method);
					if (!method.empty()) {
						currentLocation.allowMethods.push_back(method);
					}
				}
			} else if (key == "autoindex") {
				std::string value;
				if (iss >> value) {
					value = removeSemicolon(value);
					currentLocation.autoindex = (value == "on");
				}
			} else if (key == "index") {
				std::string value;
				if (iss >> value) {
					currentLocation.index = removeSemicolon(value);
				}
			} else if (key == "root") {
				std::string value;
				if (iss >> value) {
					currentLocation.root = removeSemicolon(value);
				}
			} else if (key == "return") {
				std::string value;
				if (iss >> value) {
					currentLocation.redirect = removeSemicolon(value);
				}
			} else if (key == "upload_dir") {
				std::string value;
				if (iss >> value) {
					currentLocation.uploadDir = removeSemicolon(value);
				}
			} else if (key == "cgi_path") {
				std::string path;
				currentLocation.cgiPath.clear();
				while (iss >> path) {
					path = removeSemicolon(path);
					if (!path.empty()) {
						currentLocation.cgiPath.push_back(path);
					}
				}
			} else if (key == "cgi_ext") {
				std::string ext;
				currentLocation.cgiExt.clear();
				while (iss >> ext) {
					ext = removeSemicolon(ext);
					if (!ext.empty()) {
						currentLocation.cgiExt.push_back(ext);
					}
				}
			} else {
				std::cerr << "Warning: Unknown location directive '" << key << "' (line " << lineNumber << ")" << std::endl;
			}
		} else {
			// Server-level directives
			if (key == "listen") {
				std::string value;
				if (iss >> value) {
					value = removeSemicolon(value);
					port = static_cast<uint16_t>(std::atoi(value.c_str()));
					if (port == 0) {
						std::cerr << "Error: Invalid port number '" << value << "' (line " << lineNumber << ")" << std::endl;
						file.close();
						return 1;
					}
				}
			} else if (key == "server_name") {
				std::string value;
				if (iss >> value) {
					serverName = removeSemicolon(value);
				}
			} else if (key == "host") {
				std::string value;
				if (iss >> value) {
					host = removeSemicolon(value);
				}
			} else if (key == "root") {
				std::string value;
				if (iss >> value) {
					root = removeSemicolon(value);
				}
			} else if (key == "index") {
				std::string value;
				if (iss >> value) {
					index = removeSemicolon(value);
				}
			} else if (key == "error_page") {
				int code;
				std::string page;
				if (iss >> code >> page) {
					page = removeSemicolon(page);
					errorPages[code] = page;
				} else {
					std::cerr << "Error: Invalid error_page directive format (line " << lineNumber << ")" << std::endl;
				}
			} else if (key == "client_max_body_size") {
				std::string value;
				if (iss >> value) {
					value = removeSemicolon(value);
					clientMaxBodySize = std::atoi(value.c_str());
					if (clientMaxBodySize == 0 && value != "0") {
						std::cerr << "Error: Invalid client_max_body_size '" << value << "' (line " << lineNumber << ")" << std::endl;
					}
				}
			} else {
				std::cerr << "Warning: Unknown server directive '" << key << "' (line " << lineNumber << ")" << std::endl;
			}
		}
	}
	
	// Check for unclosed blocks
	if (inServerBlock) {
		std::cerr << "Error: Unclosed server block in config file" << std::endl;
		file.close();
		return 1;
	}
	
	if (inLocationBlock) {
		std::cerr << "Error: Unclosed location block in config file" << std::endl;
		file.close();
		return 1;
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
		// Check if this is a file upload request
		std::string contentType = req.getHeader("Content-Type");
		if (contentType.find("multipart/form-data") != std::string::npos && loc && !loc->uploadDir.empty()) {
			return handleFileUpload(req, loc);
		} else {
			// Regular POST request
			response.setStatus(200, "OK");
			response.setHeader("Content-Type", "text/html");
			std::ostringstream body;
			body << "<html><body>";
			body << "<h1>POST Request Received</h1>";
			body << "<p>Request body size: " << req.getBody().length() << " bytes</p>";
			body << "<p>Content-Type: " << contentType << "</p>";
			if (!req.getBody().empty()) {
				body << "<h2>Request Body:</h2>";
				body << "<pre>" << req.getBody().substr(0, 500); // Limit display
				if (req.getBody().length() > 500) {
					body << "... (truncated)";
				}
				body << "</pre>";
			}
			body << "<a href='/'>← Back to Home</a>";
			body << "</body></html>";
			response.setBody(body.str());
			return response.toString();
		}
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

// File upload handler
std::string Server::handleFileUpload(const Request& req, const Location* loc) {
	Response response;
	
	std::string contentType = req.getHeader("Content-Type");
	std::string body = req.getBody();
	
	// Extract boundary from Content-Type
	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos == std::string::npos) {
		response.setStatus(400, "Bad Request");
		response.setBody("<html><body><h1>400 Bad Request</h1><p>No boundary found in multipart data.</p></body></html>");
		return response.toString();
	}
	
	std::string boundary = "--" + contentType.substr(boundaryPos + 9);
	
	// Find file data in multipart body
	size_t fileStart = body.find("Content-Disposition: form-data");
	if (fileStart == std::string::npos) {
		response.setStatus(400, "Bad Request");
		response.setBody("<html><body><h1>400 Bad Request</h1><p>No form data found.</p></body></html>");
		return response.toString();
	}
	
	// Extract filename
	size_t filenamePos = body.find("filename=\"", fileStart);
	if (filenamePos == std::string::npos) {
		response.setStatus(400, "Bad Request");
		response.setBody("<html><body><h1>400 Bad Request</h1><p>No filename found.</p></body></html>");
		return response.toString();
	}
	
	filenamePos += 10; // Skip 'filename="'
	size_t filenameEnd = body.find("\"", filenamePos);
	std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);
	
	// Find file content start (after headers)
	size_t contentStart = body.find("\r\n\r\n", fileStart);
	if (contentStart == std::string::npos) {
		contentStart = body.find("\n\n", fileStart);
		if (contentStart == std::string::npos) {
			response.setStatus(400, "Bad Request");
			response.setBody("<html><body><h1>400 Bad Request</h1><p>Invalid multipart format.</p></body></html>");
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
		response.setBody("<html><body><h1>400 Bad Request</h1><p>Invalid multipart format.</p></body></html>");
		return response.toString();
	}
	
	// Remove trailing CRLF before boundary
	while (contentEnd > contentStart && (body[contentEnd - 1] == '\r' || body[contentEnd - 1] == '\n')) {
		contentEnd--;
	}
	
	std::string fileContent = body.substr(contentStart, contentEnd - contentStart);
	
	// Save the file
	if (saveUploadedFile(fileContent, filename, loc->uploadDir)) {
		response.setStatus(200, "OK");
		response.setHeader("Content-Type", "text/html");
		std::ostringstream responseBody;
		responseBody << "<html><body>";
		responseBody << "<h1>File Upload Successful</h1>";
		responseBody << "<p>File '" << filename << "' has been uploaded successfully.</p>";
		responseBody << "<p>File size: " << fileContent.length() << " bytes</p>";
		responseBody << "<p>Upload directory: " << loc->uploadDir << "</p>";
		responseBody << "<a href='/'>← Back to Home</a>";
		responseBody << "</body></html>";
		response.setBody(responseBody.str());
	} else {
		response.setStatus(500, "Internal Server Error");
		response.setBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to save uploaded file.</p></body></html>");
	}
	
	return response.toString();
}

// Save uploaded file to disk
bool Server::saveUploadedFile(const std::string& content, const std::string& filename, const std::string& uploadDir) {
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

// Helper method to remove a client connection
void Server::removeClient(std::vector<struct pollfd>& fds, std::map<int, ClientConnection>& clients, int index, int& nfds) {
	int client_fd = fds[index].fd;
	close(client_fd);
	clients.erase(client_fd);
	
	// Move last element to current position
	fds[index] = fds[nfds - 1];
	nfds--;
}

// Helper method to handle client timeouts
void Server::handleClientTimeout(std::vector<struct pollfd>& fds, std::map<int, ClientConnection>& clients, int& nfds) {
	time_t currentTime = time(NULL);
	const int CLIENT_TIMEOUT = 60; // 60 seconds
	
	for (int i = 1; i < nfds; i++) {
		int client_fd = fds[i].fd;
		std::map<int, ClientConnection>::iterator it = clients.find(client_fd);
		
		if (it != clients.end() && (currentTime - it->second.lastActivity) > CLIENT_TIMEOUT) {
			std::cout << "⏰ Client timeout (fd: " << client_fd << ")" << std::endl;
			sendErrorResponse(client_fd, 408, "Request Timeout");
			removeClient(fds, clients, i, nfds);
			i--; // Adjust index after removal
		}
	}
}

// Helper method to check if HTTP request is complete
bool Server::isRequestComplete(const std::string& buffer) {
	// Check for end of headers
	size_t headerEnd = buffer.find("\r\n\r\n");
	if (headerEnd == std::string::npos) {
		headerEnd = buffer.find("\n\n");
		if (headerEnd == std::string::npos) {
			return false; // Headers not complete yet
		}
		headerEnd += 2;
	} else {
		headerEnd += 4;
	}
	
	// For GET, HEAD, DELETE requests, headers are enough
	if (buffer.find("GET ") == 0 || buffer.find("HEAD ") == 0 || buffer.find("DELETE ") == 0) {
		return true;
	}
	
	// For POST requests, check Content-Length
	if (buffer.find("POST ") == 0) {
		size_t clPos = buffer.find("Content-Length:");
		if (clPos == std::string::npos) {
			clPos = buffer.find("content-length:"); // Case insensitive
		}
		
		if (clPos != std::string::npos) {
			size_t valueStart = buffer.find(':', clPos) + 1;
			size_t valueEnd = buffer.find('\r', valueStart);
			if (valueEnd == std::string::npos) {
				valueEnd = buffer.find('\n', valueStart);
			}
			
			if (valueEnd != std::string::npos) {
				std::string lengthStr = buffer.substr(valueStart, valueEnd - valueStart);
				// Remove whitespace
				lengthStr.erase(0, lengthStr.find_first_not_of(" \t"));
				lengthStr.erase(lengthStr.find_last_not_of(" \t") + 1);
				
				int contentLength = std::atoi(lengthStr.c_str());
				return (buffer.length() >= headerEnd + contentLength);
			}
		}
		return true; // No Content-Length header, assume complete
	}
	
	return true; // Other methods, assume complete
}

// Helper method to send error responses
void Server::sendErrorResponse(int client_fd, int statusCode, const std::string& message) {
	Response resp;
	resp.setStatus(statusCode, message);
	std::ostringstream body;
	body << "<html><body><h1>" << statusCode << " " << message << "</h1></body></html>";
	resp.setBody(body.str());
	
	std::string responseStr = resp.toString();
	send(client_fd, responseStr.c_str(), responseStr.length(), MSG_NOSIGNAL);
}
