#pragma once
#include <deque>
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        return AddResultRequest(search_server_.FindTopDocuments(raw_query, document_predicate));
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool no_result = false;
    };
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& search_server_;
    int num_no_result_ = 0;
    std::vector<Document> AddResultRequest(std::vector<Document> doc);

};