#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer &search_server)
    : search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query, DocumentStatus status)
{
    auto documents = search_server_.FindTopDocuments(raw_query, status);
    UpdateRequests(documents.empty());
    return documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string &raw_query)
{
    auto documents = search_server_.FindTopDocuments(raw_query);
    RequestQueue::UpdateRequests(documents.empty());
    return documents;
}

int RequestQueue::GetNoResultRequests() const
{
    return number_of_no_result_requests_;
}

void RequestQueue::UpdateRequests(bool is_last_result_empty)
{
    if (requests_.size() == sec_in_day_)
    {
        if (requests_.front())
        {
            --number_of_no_result_requests_;
        }
        requests_.pop_front();
    }
    requests_.push_back(is_last_result_empty);
    if (requests_.back())
    {
        ++number_of_no_result_requests_;
    }
}