/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 23:02:12 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:40:38 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>
#include <fstream>
#include <cctype>
#include <cstdlib>

class Request {
public:
	Request();
	bool parse(const std::string &raw);
	std::string getMethod() const;
	std::string getPath() const;
	std::string getVersion() const;
	std::string getHeader(const std::string &key) const;
	std::string getBody() const;
	const std::map<std::string, std::string>& getHeaders() const;
	
	// Additional utility methods
	bool hasHeader(const std::string &key) const;
	size_t getContentLength() const;
	bool isChunked() const;
	bool isKeepAlive() const;
	std::string getContentType() const;
private:
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
};


#endif
