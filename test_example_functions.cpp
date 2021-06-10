#include "test_example_functions.h"
#include "log_duration.h"

using namespace std::string_literals;

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::exception& e) {
        std::cout << "Error adding document "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    try {
        LOG_DURATION_STREAM("Operation time:"s, std::cout);
        std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            std::cout << document;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error find: "s << e.what() << std::endl;
    }
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << "{ "s
        << "document_id = "s << document_id << ", "s
        << "status = "s << static_cast<int>(status) << ", "s
        << "words ="s;
    for (auto word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        LOG_DURATION_STREAM("Operation time:"s, std::cout);
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        for (const auto document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error matching documents on request "s << query << ": "s << e.what() << std::endl;
    }
}