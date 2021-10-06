#pragma once

#include "search_server.h"

template <typename Map>
bool key_compare(Map const &lhs, Map const &rhs);

void RemoveDuplicates(SearchServer &search_server);

template <typename Map>
bool key_compare(Map const &lhs, Map const &rhs)
{
    return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](auto a, auto b) { return a.first == b.first; });
}