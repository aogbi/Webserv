/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:44:49 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [configuration_file]" << std::endl;
    std::cout << "  configuration_file: Optional path to server configuration file" << std::endl;
    std::cout << "                     Default: ./configs/default.conf" << std::endl;
}

bool validateArguments(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Error: Too many arguments." << std::endl;
        printUsage(argv[0]);
        return false;
    }
    return true;
}

std::string getConfigFile(int argc, char* argv[]) {
    return (argc == 1) ? "./configs/default.conf" : argv[1];
}
