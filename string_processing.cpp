#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> result;
    size_t pos = 0;
    const size_t pos_end = text.npos;
    while (true) {
        size_t space = text.find(' ', pos);
        result.push_back(space == pos_end ? text.substr(pos) : text.substr(pos, space - pos));
        if (space == pos_end) {
            break;
        }
        else {
            pos = space + 1;
        }
    }

    return result;
}

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    const size_t pos_end = text.npos;
    while (true) {
        size_t space = text.find(' ');
        result.push_back(space == pos_end ? text : text.substr(0, space));
        if (space == pos_end) {
            break;
        }
        text.remove_prefix(space + 1);
    }

    return result;
}