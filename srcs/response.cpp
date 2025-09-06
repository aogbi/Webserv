/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/04 12:00:01 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:40:38 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "response.hpp"

Response::Response() : statusCode(200), statusMessage("OK") {}

void Response::setStatus(int code, const std::string &message) {
	statusCode = code;
	statusMessage = message;
}

void Response::setHeader(const std::string &key, const std::string &value) {
	headers[key] = value;
}

void Response::setBody(const std::string &b) {
	body = b;
}

void Response::setContentType(const std::string &mimeType) {
	setHeader("Content-Type", mimeType);
}

void Response::setRedirect(const std::string &location, int code) {
	setStatus(code, (code == 301) ? "Moved Permanently" : "Found");
	setHeader("Location", location);
}

void Response::setCookie(const std::string &name, const std::string &value, const std::string &path) {
	std::string cookieValue = name + "=" + value + "; Path=" + path;
	setHeader("Set-Cookie", cookieValue);
}

void Response::setDate() {
	time_t rawTime;
	struct tm* timeInfo;
	char buffer[80];
	
	time(&rawTime);
	timeInfo = gmtime(&rawTime);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
	
	setHeader("Date", std::string(buffer));
}

void Response::setServer(const std::string &serverName) {
	setHeader("Server", serverName);
}

int Response::getStatusCode() const {
	return statusCode;
}

std::string Response::toString() const {
	std::ostringstream resp;
	
	// Status line
	resp << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
	
	// Headers
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		resp << it->first << ": " << it->second << "\r\n";
	}
	
	// Add Content-Length if not already set
	if (headers.find("Content-Length") == headers.end()) {
		resp << "Content-Length: " << body.size() << "\r\n";
	}
	
	// Add Server header if not set
	if (headers.find("Server") == headers.end()) {
		resp << "Server: Webserv/1.0\r\n";
	}
	
	// Add Date header if not set
	if (headers.find("Date") == headers.end()) {
		time_t rawTime;
		struct tm* timeInfo;
		char buffer[80];
		
		time(&rawTime);
		timeInfo = gmtime(&rawTime);
		strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeInfo);
		
		resp << "Date: " << buffer << "\r\n";
	}
	
	// End of headers
	resp << "\r\n";
	
	// Body
	resp << body;
	
	return resp.str();
}
