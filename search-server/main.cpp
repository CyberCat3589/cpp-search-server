#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        documents_.push_back({document_id, words});
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
        const set<string> query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
        [](Document lhs, Document rhs){ 
            return lhs.relevance > rhs.relevance;
        });
    
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
    
        return matched_documents;
    }

private:
    // структура хранит ID документа и его содержание
    struct DocumentContent {
        int id = 0;
        vector<string> words;
    };
    
    vector<DocumentContent> documents_; // контейнер документов
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
    vector<Document> FindAllDocuments(const set<string>& query_words) const
    {
        vector<Document> matched_documents;
        for (const auto& document : documents_) {
            const int relevance = MatchDocument(document, query_words);
            if (relevance > 0) 
            {
                matched_documents.push_back({document.id, relevance});
            }
        }
        return matched_documents;
    }

    // функция возвращает релевантность, переданного в неё документа
    static int MatchDocument(const DocumentContent& content, const set<string>& query_words) {
        if (query_words.empty()) 
        {
            return 0;
        }
        set<string> matched_words;
        for (const string& word : content.words) 
        {
            if (matched_words.count(word) != 0) 
            {
                continue;
            }
            if (query_words.count(word) != 0) 
            {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }

    // функция разбивает запрос на ключевые слова без стоп-слов
    set<string> ParseQuery(const string& text) const
    {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) 
        {
            query_words.insert(word);
        }
        return query_words;
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