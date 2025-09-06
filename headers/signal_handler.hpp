/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   signal_handler.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/06 00:00:00 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/06 10:44:49 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <iostream>
#include <csignal>

// Forward declaration
class Server;

// Global variables - these are defined in signal_handler.cpp
extern Server* g_server;
extern volatile bool g_running;

/**
 * @brief Signal handler for graceful server shutdown
 * @param signum The signal number received
 */
void signalHandler(int signum);

/**
 * @brief Setup signal handlers for the server
 */
void setupSignalHandlers();

#endif // SIGNAL_HANDLER_HPP
