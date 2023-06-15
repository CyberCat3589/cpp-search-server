#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>

using namespace std;

// структура хранит ID документа и его релевантность
struct Document
{
    int id;
    int relevance;
};

// функция производит чтение введенной строки
string ReadLine() 
{
    string s;
    getline(cin, s);
    return s;
}

// функция производит чтение введенного числа
int ReadLineWithNumber() 
{
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

// кол-во возвращаемых документов в FindTopDocuments
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer 
{
public:
    // функция добавляет новый документ в контейнер локументов
    void AddDocument(int document_id, const string& document) 
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        for(string word : words)
        {
            word_to_documents_[word].insert(document_id);
        }
    }

    // функция добавляет стоп-слова из строки в контейнер стоп-слов
    void SetStopWords(const string& text) 
    {
        for (const string& word : SplitIntoWords(text)) 
        {
            stop_words_.insert(word);
        }
    }

    // Возвращает топ-5 самых релевантных документов в виде пар: {id, релевантность}
    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query);

        sort(matched_documents.begin(), matched_documents.end(),
        [](Document lhs, Document rhs)
        { 
            return lhs.relevance > rhs.relevance;
        });
    
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
    
        return matched_documents;
    }

private:

    // структура хранит плюс- и минус-слова запроса
    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    map<string, set<int>> word_to_documents_; // словарь содержит слова и в каких док-тах они встречаются
    set<string> stop_words_; // контейнер стоп-слов

    // функция разбивает строку на слова
    vector<string> SplitIntoWords(const string& text) const
    {
        vector<string> words;
        string word;
        for (const char c : text) {
            if (c == ' ') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else 
            {
                word += c;
            }
        }
        if (!word.empty()) 
        {
            words.push_back(word);
        }

        return words;
    }

    // функция разбивает строку на слова, игнорируя стоп-слова
    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (stop_words_.count(word) == 0) {
                words.push_back(word);
            }
        }
        return words;
    }

    // функция для каждого документа возвращает его релевантность и id
    vector<Document> FindAllDocuments(const Query& query) const
    {
        vector<Document> matched_documents;
        
        map<int, int> document_to_relevance;
        set<int> identificators;
        for(string plus_word : query.plus_words)
        {
            if(word_to_documents_.count(plus_word) > 0)
            {
                identificators = word_to_documents_.at(plus_word);
                for(int id : identificators)
                {
                    document_to_relevance[id] += 1;
                }
            }
            identificators.clear();
        }
        
        for(string minus_word : query.minus_words)
        {
            if(word_to_documents_.count(minus_word) > 0)
            {
                identificators = word_to_documents_.at(minus_word);
                for(int id : identificators)
                {
                    document_to_relevance.erase(id);
                }
            }
            identificators.clear();
        }
        
        for(auto [id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({id, relevance});
        }
        
        return matched_documents;
    }

    // функция разбивает запрос на ключевые слова без стоп-слов
    Query ParseQuery(const string& text) const
    {
        Query query;
        for (const string& word : SplitIntoWordsNoStop(text)) 
        {
            if(word[0] == '-')
            {
                query.minus_words.insert(word.substr(1));
            }
            else
            {
                query.plus_words.insert(word);
            }
        }
        
        return query;
    }
};

// функция считывает стоп-слова и документы из стандартного ввода и возвращает готовый экземрляр SearchServer 
SearchServer CreateSearchServer()
{
    SearchServer server;

    const string stop_words_joined = ReadLine();
    server.SetStopWords(stop_words_joined);

    // Чтение документов
    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) 
    {
        server.AddDocument(document_id, ReadLine());
    }

    return server;
}

int main() 
{
    const SearchServer srv = CreateSearchServer();

    const string query = ReadLine();
    for (auto [document_id, relevance] : srv.FindTopDocuments(query)) 
    {
        cout << "{ document_id = "s << document_id << ", relevance = "s << relevance << " }"s << endl;
    }
}