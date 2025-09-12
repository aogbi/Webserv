/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/12 17:47:27 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <iostream>
#include <string>

/**
 * @brief Print usage information for the program
 * @param programName The name of the program executable
 */
void printUsage(const char* programName);

/**
 * @brief Validate command line arguments
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return true if arguments are valid, false otherwise
 */
bool validateArguments(int argc, char* argv[]);

std::string getConfigFile(int argc, char* argv[]);

#endif
