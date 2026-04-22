#include "includes/utils.h"

std::string get_link_from_music_id(int music_id, std::string format)
{
	return fmt::format("https://aicon.ngfiles.com/{}/{}.{}", std::to_string(music_id / 1000), std::to_string(music_id), format);
}

// Taken from https://stackoverflow.com/questions/154536/encode-decode-urls-in-c
std::string url_encode(const std::string &value) {
    std::stringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}