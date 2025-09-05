/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 23:02:46 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:40:38 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request.hpp"

Request::Request() {}

bool Request::parse(const std::string &raw) {
	// Clear any previous data
	method.clear();
	path.clear();
	version.clear();
	headers.clear();
	body.clear();
	
	if (raw.empty()) {
		return false;
	}
	
	// Find the end of headers (double CRLF or double LF)
	size_t headerEnd = raw.find("\r\n\r\n");
	bool useCRLF = true;
	
	if (headerEnd == std::string::npos) {
		headerEnd = raw.find("\n\n");
		useCRLF = false;
		if (headerEnd == std::string::npos) {
			// No body separator found, treat entire request as headers
			headerEnd = raw.length();
		}
	}
	
	// Parse headers section
	std::string headerSection = raw.substr(0, headerEnd);
	std::istringstream stream(headerSection);
	std::string line;
	
	// Parse request line
	if (!std::getline(stream, line)) {
		return false;
	}
	
	// Remove trailing \r if present
	if (!line.empty() && line[line.length() - 1] == '\r') {
		line.erase(line.length() - 1);
	}
	
	std::istringstream firstLine(line);
	if (!(firstLine >> method >> path >> version)) {
		return false;
	}
	
	// Parse headers
	while (std::getline(stream, line)) {
		// Remove trailing \r if present
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		
		if (line.empty()) {
			break; // End of headers
		}
		
		size_t colon = line.find(":");
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			
			// Trim leading whitespace from value
			while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
				value.erase(0, 1);
			}
			
			// Trim trailing whitespace from value
			while (!value.empty() && (value[value.length()-1] == ' ' || value[value.length()-1] == '\t')) {
				value.erase(value.length()-1);
			}
			
			// Convert header name to lowercase for case-insensitive comparison
			for (size_t i = 0; i < key.length(); ++i) {
				key[i] = std::tolower(key[i]);
			}
			
			headers[key] = value;
		}
	}
	
	// Parse body if present
	if (headerEnd < raw.length()) {
		size_t bodyStart = headerEnd + (useCRLF ? 4 : 2);
		if (bodyStart < raw.length()) {
			body = raw.substr(bodyStart);
		}
	}
	
	return true;
}

std::string Request::getMethod() const {
	return method;
}

std::string Request::getPath() const {
	return path;
}

std::string Request::getVersion() const {
	return version;
}

std::string Request::getHeader(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = headers.find(key);
	if (it != headers.end())
		return it->second;
	return "";
}

std::string Request::getBody() const {
	return body;
}

const std::map<std::string, std::string>& Request::getHeaders() const {
	return headers;
}

bool Request::hasHeader(const std::string &key) const {
	std::string lowerKey = key;
	for (size_t i = 0; i < lowerKey.length(); ++i) {
		lowerKey[i] = std::tolower(lowerKey[i]);
	}
	return headers.find(lowerKey) != headers.end();
}

size_t Request::getContentLength() const {
	std::string contentLength = getHeader("content-length");
	if (contentLength.empty()) {
		return 0;
	}
	return static_cast<size_t>(atol(contentLength.c_str()));
}

bool Request::isChunked() const {
	std::string transferEncoding = getHeader("transfer-encoding");
	return transferEncoding.find("chunked") != std::string::npos;
}

bool Request::isKeepAlive() const {
	std::string connection = getHeader("connection");
	if (version == "HTTP/1.1") {
		// HTTP/1.1 defaults to keep-alive unless explicitly closed
		return connection != "close";
	} else {
		// HTTP/1.0 defaults to close unless explicitly keep-alive
		return connection == "keep-alive";
	}
}

std::string Request::getContentType() const {
	return getHeader("content-type");
}


