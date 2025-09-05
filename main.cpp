/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:56:58 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/05 16:43:45 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <iostream>
#include <csignal>

Server* g_server = NULL;

void signalHandler(int signum) {
    if (g_server) {
        std::cout << "\n🛑 Signal " << signum << " received. Shutting down server gracefully..." << std::endl;
        delete g_server;
        g_server = NULL;
    }
    exit(signum);
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [configuration_file]" << std::endl;
    std::cout << "  configuration_file: Optional path to server configuration file" << std::endl;
    std::cout << "                     Default: ./configs/default.conf" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cerr << "Error: Too many arguments." << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    
    try {
        std::string configFile = (argc == 1) ? "./configs/default.conf" : argv[1];
        std::cout << "🌐 Starting Webserv..." << std::endl;
        std::cout << "📁 Using config file: " << configFile << std::endl;
        g_server = new Server(configFile);
        if (!g_server->setup()) {
            std::cerr << "❌ Error: Failed to setup server" << std::endl;
            delete g_server;
            return 1;
        }
        std::cout << "✅ Server setup complete. Running..." << std::endl;
        g_server->run();
        delete g_server;
        g_server = NULL;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal Error: " << e.what() << std::endl;
        if (g_server) {
            delete g_server;
            g_server = NULL;
        }
        return 1;
    } catch (...) {
        std::cerr << "❌ Fatal Error: Unknown exception occurred" << std::endl;
        if (g_server) {
            delete g_server;
            g_server = NULL;
        }
        return 1;
    }
    
    return 0;
}
