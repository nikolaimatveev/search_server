#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> result;
    std::size_t start = 0, end = 0;
    while ((end = text.find(' ', start)) != std::string::npos) {
        result.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    result.push_back(text.substr(start));
    return result;
}

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> result;
    std::size_t end = 0;
    while ((end = text.find(' ')) != std::string_view::npos) {
        result.push_back(text.substr(0, end));
        text.remove_prefix(end + 1);
    }
    result.push_back(text);
    return result;
}