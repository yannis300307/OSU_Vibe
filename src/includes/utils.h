#pragma once

#include <string>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

std::string get_link_from_music_id(int music_id, std::string format);
std::string url_encode(const std::string &value);