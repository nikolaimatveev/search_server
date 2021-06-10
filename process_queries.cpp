#include "process_queries.h"
#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    std::vector<std::vector<Document>> result(queries.size());
    result.reserve(queries.size());
    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](const std::string& query)->std::vector<Document> {
            return search_server.FindTopDocuments(query);
        });

    return result;
}

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
    auto const tmp = ProcessQueries(search_server, queries);

    std::list<Document> result;

    for(const auto& reply : tmp) {
        result.insert(result.end(), reply.begin(), reply.end());
    }

    return result;
}