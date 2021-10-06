#pragma once
#include <algorithm>
#include <execution>

#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"

using namespace std::string_literals;

static const int MAX_RESULT_DOCUMENT_COUNT = 5;

static const int NUMBER_PARALLEL_PROCESSES = 4;

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer
{
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words);

    explicit SearchServer(const std::string &stop_words_text);

    explicit SearchServer(const std::string_view &stop_words_text);

    void AddDocument(int document_id, const std::string_view &document, DocumentStatus status, const std::vector<int> &ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view &raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view &raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view &raw_query) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy &execution_policy, const std::string_view &raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy &, const std::string_view &raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy &, const std::string_view &raw_query) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy &, const std::string_view &raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy &, const std::string_view &raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view &raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy &, std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy &, std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double> GetWordFrequencies(int document_id);

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &, int document_id);

private:
    struct DocumentData
    {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view &word) const;

    static bool IsValidWord(const std::string_view &word);

    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view &text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord
    {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view &text) const;

    struct Query
    {
        std::set<std::string_view, std::less<>> plus_words;
        std::set<std::string_view, std::less<>> minus_words;
    };

    Query ParseQuery(const std::vector<std::string_view> &query) const;

    double ComputeWordInverseDocumentFreq(const std::string_view &word) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy &execution_policy, const Query &query, DocumentPredicate document_predicate) const;
};

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view> &words, DocumentStatus status);

void AddDocument(SearchServer &search_server, int document_id, const std::string_view &document, DocumentStatus status,
                 const std::vector<int> &ratings);

void FindTopDocuments(const SearchServer &search_server, const std::string_view &raw_query);

void MatchDocuments(const SearchServer &search_server, const std::string_view &query);

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view &raw_query, DocumentPredicate document_predicate) const
{
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy &execution_policy, const std::string_view &raw_query, DocumentPredicate document_predicate) const
{
    std::vector<std::string_view> vec_query = SplitIntoWordsView(raw_query);
    const Query query = ParseQuery(vec_query);

    auto matched_documents = FindAllDocuments(execution_policy, query, document_predicate);

    sort(execution_policy, matched_documents.begin(), matched_documents.end(), [](const Document &lhs, const Document &rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < 1e-6)
        {
            return lhs.rating > rhs.rating;
        }
        else
        {
            return lhs.relevance > rhs.relevance;
        }
    });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy &execution_policy, const Query &query, DocumentPredicate document_predicate) const
{
    ConcurrentMap<int, double> document_to_relevance(NUMBER_PARALLEL_PROCESSES);
    for (const std::string_view &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        std::for_each(execution_policy, word_to_document_freqs_.find(word)->second.begin(), word_to_document_freqs_.find(word)->second.end(),
                      [&](const auto doc_id_word_freqs) {const auto &document_data = documents_.at(doc_id_word_freqs.first);
            if (document_predicate(doc_id_word_freqs.first, document_data.status, document_data.rating))
            {
                document_to_relevance[doc_id_word_freqs.first].ref_to_value += doc_id_word_freqs.second * inverse_document_freq;
            } });
    }

    for (const std::string_view &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        std::for_each(execution_policy, word_to_document_freqs_.find(word)->second.begin(), word_to_document_freqs_.find(word)->second.end(),
                      [&](const auto doc_id_word_freqs) { document_to_relevance.BuildOrdinaryMap().erase(doc_id_word_freqs.first); });
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap())
    {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}