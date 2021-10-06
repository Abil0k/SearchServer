#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer &search_server)
{
    std::set<int> deleted_documents;
    for (const int document_id_out : search_server)
    {
        for (const int document_id_in : search_server)
        {
            if (document_id_out != document_id_in)
            {
                if (key_compare(search_server.GetWordFrequencies(document_id_out), search_server.GetWordFrequencies(document_id_in)))
                {
                    if (document_id_out > document_id_in)
                    {
                        deleted_documents.insert(document_id_out);
                    }
                    else if (document_id_out < document_id_in)
                    {
                        deleted_documents.insert(document_id_in);
                    }
                }
            }
        }
    }
    for (const int deleted_id : deleted_documents)
    {
        search_server.RemoveDocument(deleted_id);
        std::cout << "Found duplicate document id " << deleted_id << std::endl;
    }
}