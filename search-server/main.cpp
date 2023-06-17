#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

// структура хранит ID документа и его релевантность
struct Document
{
    int id;
    double relevance;
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
        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);

        //TF
        double term_frequency = 1.0 / words.size();

        for(string word : words)
        {
            word_to_document_fregs_[word][document_id] += term_frequency;
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

    // хранит слово из запроса и его признаки: стоп и минус
    struct QueryWord
    {
        string word;
        bool is_minus;
        bool is_stop_word;
    };

    // структура хранит плюс- и минус-слова запроса
    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    map<string, map<int, double>> word_to_document_fregs_; // словарь содержит слова и в каких док-тах они встречаются
    set<string> stop_words_; // контейнер стоп-слов
    int document_count_ = 0; // кол-во документов

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    }

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
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // функция для каждого документа возвращает его релевантность и id
    vector<Document> FindAllDocuments(const Query& query) const
    {
        vector<Document> matched_documents;
        
        map<int, double> document_to_relevance; // словарь: ключ - id документа, значение - релевантность
        map<int, double> identificator_and_tf; // контейнер с id документов, в содержится слово из запроса

        // поиск плюс-слов среди документов
        for(string plus_word : query.plus_words)
        {
            if(word_to_document_fregs_.count(plus_word) > 0)
            {
                identificator_and_tf = word_to_document_fregs_.at(plus_word);

                // IDF
                double inverse_document_frequency = log(static_cast<double>(document_count_) / identificator_and_tf.size());

                for(auto [id, tf] : identificator_and_tf)
                {
                    double relevance = inverse_document_frequency * tf;
                    document_to_relevance[id] += relevance;
                }
            }
        }
        
        // поиск минус-слов среди документов
        vector<int> numbers_to_delete; // промежуточный контейнер для удаления
        for(string minus_word : query.minus_words)
        {
            if(word_to_document_fregs_.count(minus_word) > 0)
            {
                identificator_and_tf = word_to_document_fregs_.at(minus_word);
                for(auto [id, tf] : identificator_and_tf)
                {
                    // запись ID документа, который необходимо удалить в промежуточный контейнер
                    numbers_to_delete.push_back(id);
                }
            }

            // удаление документов, содержащих минус-слова из списка релевантных документов
            for(int num : numbers_to_delete)
            {
                document_to_relevance.erase(num);
            }
        }
        
        // запись результата в результирующий контейнер
        for(auto [id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({id, static_cast<double>(relevance)});
        }
        
        return matched_documents;
    }

    // возвращает слово из запроса и его признаки: стоп и минус
    QueryWord ParseQueryWord(string text) const
    {
        bool is_minus_word = false;

        if(text[0] == '-')
        {
            is_minus_word = true;
            text = text.substr(1);
        }

        return {text, is_minus_word, IsStopWord(text)};
    }

    // функция разбивает запрос на ключевые слова без стоп-слов
    Query ParseQuery(const string& text) const
    {
        Query query;
        for (const string& word : SplitIntoWordsNoStop(text)) 
        {
            const QueryWord query_word = ParseQueryWord(word);
            if(!query_word.is_stop_word)
            {
                if(query_word.is_minus)
                {
                    query.minus_words.insert(query_word.word);
                }
                else
                {
                    query.plus_words.insert(query_word.word);
                }
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
    int document_count = ReadLineWithNumber();

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