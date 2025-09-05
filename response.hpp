/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/04 12:00:17 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:40:38 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <ctime>

class Response {
public:
	Response();
	void setStatus(int code, const std::string &message);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);
	std::string toString() const;
	
	// Additional utility methods
	void setContentType(const std::string &mimeType);
	void setRedirect(const std::string &location, int code = 301);
	void setCookie(const std::string &name, const std::string &value, const std::string &path = "/");
	void setDate();
	void setServer(const std::string &serverName = "Webserv/1.0");
	int getStatusCode() const;
private:
	int statusCode;
	std::string statusMessage;
	std::map<std::string, std::string> headers;
	std::string body;
};

#endif

