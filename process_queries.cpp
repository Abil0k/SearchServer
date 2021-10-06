#include "process_queries.h"
#include <functional>
#include <numeric>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::vector<std::vector<Document>> documents(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), documents.begin(),
                   [&search_server](const std::string &query) {
                       return search_server.FindTopDocuments(query);
                   });
    return documents;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer &search_server, const std::vector<std::string> &queries)
{
    std::vector<std::vector<Document>> vector_documents = ProcessQueries(search_server, queries);
    size_t size = std::transform_reduce(std::execution::par, vector_documents.begin(), vector_documents.end(), 0, std::plus<>{}, [](std::vector<Document> &doc) { return doc.size(); });
    std::vector<Document> documents(size);
    auto it = documents.begin();
    for (const auto &document : vector_documents)
    {
        std::transform(std::execution::par, document.begin(), document.end(), it, [](auto &doc) { return doc; });
        it += document.size();
    }
    return documents;
}