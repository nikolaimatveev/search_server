#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    return AddResultRequest(search_server_.FindTopDocuments(raw_query, status));
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    return AddResultRequest(search_server_.FindTopDocuments(raw_query));
}

int RequestQueue::GetNoResultRequests() const {
    return num_no_result_;
}

std::vector<Document> RequestQueue::AddResultRequest(std::vector<Document> doc) {
    bool no_result = false;
    if (doc.empty()) {
        no_result = true;
        ++num_no_result_;
    }

    if (requests_.size() == sec_in_day_) {
        if (requests_.front().no_result) {
            --num_no_result_;
        }
        requests_.pop_front();
    }
    requests_.push_back({ no_result });
    return doc;
}