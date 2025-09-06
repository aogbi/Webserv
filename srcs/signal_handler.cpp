/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_handler.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:44:49 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "signal_handler.hpp"
#include "server.hpp"

Server* g_server = NULL;
volatile bool g_running = true;

void signalHandler(int signum) {
    std::cout << "\n🛑 Signal " << signum << " received. Shutting down server gracefully..." << std::endl;
    g_running = false;
}

void setupSignalHandlers() {
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
}
