/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/04 12:00:01 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/04 12:02:48 by aogbi            ###   ########.fr       */
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

std::string Response::toString() const {
	std::ostringstream resp;
	resp << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		resp << it->first << ": " << it->second << "\r\n";
	}
	resp << "Content-Length: " << body.size() << "\r\n";
	resp << "\r\n";
	resp << body;
	return resp.str();
}

