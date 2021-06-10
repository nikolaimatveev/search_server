#pragma once
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <execution>
#include <type_traits>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include "log_duration.h"

using namespace std::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const int BUCKET_COUNT = 3;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            using namespace std::string_literals;
            throw std::invalid_argument("Stop words contain invalid characters");
        }
    }

    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(policy, query, document_predicate);

        std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
            });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
            });
    }

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query) const {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    //На вход поступает запрос и id документа
    //Возвращаеться кортедж и в первом элементе все плюс-слова запроса (без дублирования), содержащиеся в документе. 
    //Если документ не соответствует запросу(нет пересечений по плюс - словам или есть минус - слово), вектор слов нужно вернуть пустым.
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    
    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
        if ((document_id < 0) || (document_ids_.count(document_id) == 0)) {
            throw std::invalid_argument("Document id: ["s + std::to_string(document_id) + "] is invalid"s);
        }

        const auto query = ParseQuery(raw_query);

        std::vector<std::string_view> matched_words;

        std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), 
            [&matched_words,
            &document_id,
            &word_to_document_freqs_ = word_to_document_freqs_](std::string_view word) {
                if (word_to_document_freqs_.count(word) == 1) {
                    if (word_to_document_freqs_.at(word).count(document_id)) {
                        matched_words.push_back(word);
                    }
                }
            }
        );

        std::for_each(policy, query.minus_words.begin(), query.minus_words.end(),
            [&matched_words,
            &document_id,
            &word_to_document_freqs_ = word_to_document_freqs_](std::string_view word) {
                if (word_to_document_freqs_.count(word) == 1) {
                    if (word_to_document_freqs_.at(word).count(document_id)) {
                        matched_words.clear();
                    }
                }
            }
        );

        return { matched_words, documents_.at(document_id).status };
    }
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);

    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id) {
        const auto it = documents_to_word_freqs_.find(document_id);//O(log(N))
        if (it == documents_to_word_freqs_.end()) {
            return;
        }

        std::for_each(policy, it->second.begin(), it->second.end(),
            [&document_id, &word_to_document_freqs_ = word_to_document_freqs_](const std::pair<std::string_view, double>& value) {
                const auto word = word_to_document_freqs_.find(value.first);//O(log(N))
                word->second.erase(document_id);
            });

        documents_to_word_freqs_.erase(it);
        document_ids_.erase(document_id);
        documents_.erase(document_id);
    }
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::set<std::string> source_words_;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::map<int, std::map<std::string_view, double>> documents_to_word_freqs_;
    std::set<int> document_ids_;

    bool IsStopWord(std::string_view word) const;

    static bool IsValidWord(std::string_view word);

    std::vector<std::string> SplitIntoWordsNoStop(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy policy, const Query& query, DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_unsequenced_policy>) {
            ConcurrentMap<int, double> tmp(BUCKET_COUNT);
            for (std::string_view word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        tmp[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
            document_to_relevance = std::move(tmp.BuildOrdinaryMap());
        } else {
            for (std::string_view word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id] += term_freq * inverse_document_freq;
                    }
                }
            }
        }
        for (std::string_view word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};