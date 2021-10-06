#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string_view &text)
{
    std::vector<std::string> words;
    std::string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(std::move(word));
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
    {
        words.push_back(std::move(word));
    }

    return words;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view text)
{
    std::vector<std::string_view> words;
    while (true)
    {
        int64_t space = text.find(' ', 0);
        words.push_back(space == static_cast<int64_t>(text.npos) ? text.substr(0) : text.substr(0, space - 0));
        if (space == static_cast<int64_t>(text.npos))
        {
            break;
        }
        else
        {
            text.remove_prefix(space + 1);
        }
    }
    return words;
}