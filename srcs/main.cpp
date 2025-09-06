/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:56:58 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:44:49 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include "signal_handler.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <exception>

/**
 * @brief Initialize and run the web server
 * @param configFile Path to the configuration file
 * @return true on successful execution, false on error
 */
bool initializeAndRunServer(const std::string& configFile) {
    std::cout << "🌐 Starting Webserv..." << std::endl;
    std::cout << "📁 Using config file: " << configFile << std::endl;
    
    g_server = new Server(configFile);
    if (!g_server->setup()) {
        std::cerr << "❌ Error: Failed to setup server" << std::endl;
        delete g_server;
        g_server = NULL;
        return false;
    }
    
    std::cout << "✅ Server setup complete. Running..." << std::endl;
    g_server->run();
    std::cout << "🛑 Server shutdown complete." << std::endl;
    
    return true;
}

/**
 * @brief Clean up server resources
 */
void cleanup() {
    if (g_server) {
        delete g_server;
        g_server = NULL;
    }
}

int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (!validateArguments(argc, argv)) {
        return 1;
    }
    
    // Setup signal handlers for graceful shutdown
    setupSignalHandlers();
    
    try {
        std::string configFile = getConfigFile(argc, argv);
        
        if (!initializeAndRunServer(configFile)) {
            return 1;
        }
        
        cleanup();
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal Error: " << e.what() << std::endl;
        cleanup();
        return 1;
    } catch (...) {
        std::cerr << "❌ Fatal Error: Unknown exception occurred" << std::endl;
        cleanup();
        return 1;
    }
    
    return 0;
}
