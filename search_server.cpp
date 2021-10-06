#include <cmath>

#include "search_server.h"
#include "log_duration.h"

using namespace std::string_literals;

SearchServer::SearchServer(const std::string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

SearchServer::SearchServer(const std::string_view &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view &document, DocumentStatus status, const std::vector<int> &ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string &word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view &raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view &raw_query) const
{
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy &, const std::string_view &raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy &, const std::string_view &raw_query) const
{
    return SearchServer::FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy &, const std::string_view &raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy &, const std::string_view &raw_query) const
{
    return SearchServer::FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view &raw_query, int document_id) const
{
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy &, std::string_view raw_query, int document_id) const
{
    std::vector<std::string_view> vec_query = SplitIntoWordsView(raw_query);
    const Query query = SearchServer::ParseQuery(vec_query);
    std::vector<std::string_view> matched_words;
    for (const std::string_view &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.find(word)->second.count(document_id))
        {
            matched_words.push_back(word);
        }
    }
    for (const std::string_view &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.find(word)->second.count(document_id))
        {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy &, std::string_view raw_query, int document_id) const
{
    std::vector<std::string_view> vec_query = SplitIntoWordsView(raw_query);
    const Query query = SearchServer::ParseQuery(vec_query);
    std::vector<std::string_view> matched_words;
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(), back_inserter(matched_words),
                 [&](const std::string_view &word) { return (word_to_document_freqs_.count(word) == 0) ? false : (word_to_document_freqs_.find(word)->second.count(document_id)); });
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
                    [&](const std::string_view &word) { return (word_to_document_freqs_.count(word) == 0) ? false : (word_to_document_freqs_.find(word)->second.count(document_id)); }))
    {
        matched_words.clear();
    }
    return {matched_words, documents_.at(document_id).status};
}

const std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id)
{
    std::map<std::string_view, double> word_frequencies;
    for (const auto &[word, id_freq] : word_to_document_freqs_)
    {
        if (id_freq.count(document_id))
        {
            word_frequencies.emplace(word, id_freq.at(document_id));
        }
    }
    return word_frequencies;
}

void SearchServer::RemoveDocument(int document_id)
{
    RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &, int document_id)
{
    for (const auto &[word, id_freq] : word_to_document_freqs_)
    {
        if (id_freq.count(document_id))
        {
            word_to_document_freqs_.at(word).erase(document_id);
        }
    }
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &, int document_id)
{
    std::for_each(std::execution::par, word_to_document_freqs_.begin(), word_to_document_freqs_.end(), [&](const auto &word_id_freqs) { word_to_document_freqs_.at(word_id_freqs.first).erase(document_id); });
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

bool SearchServer::IsStopWord(const std::string_view &word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view &word)
{
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view &text) const
{
    std::vector<std::string> words;
    for (const std::string &word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw std::invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(std::move(word));
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings)
    {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view &text) const
{
    if (text.empty())
    {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word{text};
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        std::string txt{text};
        throw std::invalid_argument("Query word "s + txt + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::vector<std::string_view> &query) const
{
    Query result;
    for (const std::string_view &word : query)
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.insert(std::move(query_word.data));
            }
            else
            {
                result.plus_words.insert(std::move(query_word.data));
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view &word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.find(word)->second.size());
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view> &words, DocumentStatus status)
{
    std::cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (const std::string_view &word : words)
    {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer &search_server, int document_id, const std::string_view &document, DocumentStatus status,
                 const std::vector<int> &ratings)
{
    try
    {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer &search_server, const std::string_view &raw_query)
{
    LOG_DURATION("Operation time"s, std::cerr);
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try
    {
        for (const Document &document : search_server.FindTopDocuments(raw_query))
        {
            std::cout << document << std::endl;
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer &search_server, const std::string_view &query)
{
    LOG_DURATION("Operation time"s, std::cerr);
    try
    {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        for (const int document_id : search_server)
        {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}