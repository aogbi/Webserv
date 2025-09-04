/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/19 18:56:58 by aogbi             #+#    #+#             */
/*   Updated: 2025/09/04 11:30:29 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"

int main(int argc, char *argv[]) {
    if (argc == 1 || argc == 2)
    {
        Server server((argc == 1) ? "./configs/default.conf" : argv[1]);
        if (!server.setup()) {
            return 1;
        }
        server.run();
    }
    return 0;
}
