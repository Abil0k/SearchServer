#pragma once
#include <vector>
#include <deque>
#include <string>

#include "search_server.h"
#include "document.h"

class RequestQueue
{
public:
    explicit RequestQueue(const SearchServer &search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    int GetNoResultRequests() const;

private:
    void UpdateRequests(bool is_last_result_empty);

    std::deque<bool> requests_;
    const static int sec_in_day_ = 1440;
    int number_of_no_result_requests_ = 0;
    const SearchServer &search_server_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate)
{
    auto documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    UpdateRequests(documents.empty());
    return documents;
}