
#include <cmath>
#include <utility>

#include "search_server.h"
#include "string_processing.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                     // from string container
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(std::string(stop_words_text)))
{
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Document id: ["s + std::to_string(document_id) + "] is invalid"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        auto [it,_] = source_words_.emplace(word);
        word_to_document_freqs_[*it][document_id] += inv_word_count;
        documents_to_word_freqs_[document_id][*it] += inv_word_count;
    }
    document_ids_.insert(document_id);
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::set<int>::const_iterator SearchServer::begin() const {//O(1)
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {//O(1)
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    const auto it = documents_to_word_freqs_.find(document_id);//O(N)
    if (it != documents_to_word_freqs_.end()) {
        return it->second;
    }
    else {
        static const std::map<std::string_view, double> dummy;
        return dummy;
    }
}

void SearchServer::RemoveDocument( int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(),
        [](char c) {
            return c >= '\0' && c < ' ';
        }
    );
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word from document ["s + std::string(word) + "] is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(std::string(word));
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word ["s + std::string(text) + "] is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}


SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
