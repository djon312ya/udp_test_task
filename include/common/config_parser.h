/*
 * config_parser.h
 *
 *  Created on: Aug 3, 2025
 *      Author: djon
 */
#ifndef _COMMON_CONFIG_PARSER_H_
#define _COMMON_CONFIG_PARSER_H_

#include <string>
#include <map>
#include <fstream>
#include <sstream>

inline std::map<std::string, std::string> parse_config(const std::string& filename) {
    std::map<std::string, std::string> config_map;
    std::ifstream config_file(filename);
    if (!config_file.is_open()) {
        throw std::runtime_error("Could not open config file: " + filename);
    }
    std::string line;
    while (std::getline(config_file, line)) {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                config_map[key] = value;
            }
        }
    }
    return config_map;
}

#endif /* COMMON_CONFIG_PARSER_H_ */
