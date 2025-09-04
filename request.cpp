/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 23:02:46 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/04 11:51:30 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "request.hpp"

Request::Request() {}

bool Request::parse(const std::string &raw) {
	std::istringstream stream(raw);
	std::string line;
	if (!std::getline(stream, line))
		return false;
	std::istringstream firstLine(line);
	if (!(firstLine >> method >> path >> version))
		return false;
	bool headersDone = false;
	while (std::getline(stream, line)) {
		if (!headersDone) {
			if (line.empty() || line == "\r") {
				headersDone = true;
				continue;
			}
			size_t colon = line.find(":");
			if (colon != std::string::npos) {
				std::string key = line.substr(0, colon);
				std::string value = line.substr(colon + 1);
				while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
					value.erase(0, 1);
				if (!value.empty() && value[value.size()-1] == '\r')
					value.erase(value.size()-1);
				headers[key] = value;
			}
		} else {
			if (!body.empty())
				body += "\n";
			body += line;
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


