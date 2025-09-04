/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 23:02:12 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/04 11:51:30 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <iostream>
#include <map>
#include <sstream>

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
private:
	std::string method;
	std::string path;
	std::string version;
	std::map<std::string, std::string> headers;
	std::string body;
};


#endif
