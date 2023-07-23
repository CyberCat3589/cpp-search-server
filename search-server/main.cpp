#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <math.h>
#include <optional>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

// считывание строки из ввода
string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

// считывание числа из ввода
int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

// разбиение строки на слова
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document 
{
    Document() = default;

    Document(int id_in, double relevance_in, int rating_in) : id(id_in), relevance(relevance_in), rating(rating_in) {}

    int id = 0;
    double relevance = 0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    explicit SearchServer(const string& text)
    {
        SearchServer(SplitIntoWords(text));
    }

    template <typename StringContainer>
    explicit SearchServer(StringContainer container)
    {
        for(string word : container)
        {
            if(!IsValidWord(word))
            {
                throw invalid_argument("Переданное стоп-слово содержит спец. символы!!!"s);
            }
            if(!word.empty() && (stop_words_.count(word) == 0)) stop_words_.insert(word);
        }
    }

    // добавление нового документа
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) 
    {
        if(!IsValidDocumentID(document_id)) throw invalid_argument("Недопустимый id документа!!!"s);
        if(IsValidWord(document)) throw invalid_argument("Текст документа содержит спец. символы!!!"s);

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size(); // расчет term frequency
        for (const string& word : words) 
        {
            word_to_document_freqs_[word][document_id] += inv_word_count; 
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    // возвращает MAX_RESULT_DOCUMENT_COUNT документов с наибольшей релевантностью/рейтингом
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const 
    {
        if(CheckQuery(raw_query) || !IsValidWord(raw_query)) throw invalid_argument("Недопустимый запрос!!!"s); 

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance || ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) && lhs.rating > rhs.rating);
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const 
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) 
            {
                return document_status == status;
            });
    }
 
    vector<Document> FindTopDocuments(const string& raw_query) const 
    {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    // количество документов в системе
    int GetDocumentCount() const
    {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const 
    {
        if(CheckQuery(raw_query)) throw invalid_argument("Недопустимый запрос!!!"s);
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) 
        {
            if (!IsValidWord(word)) throw invalid_argument("Недопустимый запрос!!!"s);;
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) 
            {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) 
        {
            if (!IsValidWord(word)) throw invalid_argument("Недопустимый запрос!!!"s);;
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) 
            {
                matched_words.clear();
                break;
            }
        }

        return tuple{matched_words, documents_.at(document_id).status};
    }

    int GetDocumentId(int index) const 
    {
        if (index > documents_.size() || index < 0) throw out_of_range("Недопустимый индекс документа!!!"s);
        int i = 0;
        int t_id = 0;
        for (const auto [id, bruh] : documents_)
        {
            if (i == index)
                return id;
            i++;
            t_id = id;
        }
        return t_id;
    }
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_; // слова, id документов, tf
    map<int, DocumentData> documents_; // id, рейтинг, статус

    static bool CheckQuery(const string& text) 
    {
        for (int i = 0; i < text.size(); i++)
        {
            if (text[i] == '-' && (i == 0 || text[i - 1] == '-' || i == text.size() - 1 || (text[i + 1] && text[i + 1] == ' ')))
                return true;
        }
        return false;
    }

    bool IsValidDocumentID(int document_id) const 
    {
        return !(documents_.count(document_id) > 0 || document_id < 0);
    }
    
    static bool IsValidWord(const string& word) 
    {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) 
        {
            return c >= '\0' && c < ' ';
        });
    }

    bool IsStopWord(const string& word) const 
    {
        return stop_words_.count(word) > 0;
    }

    // разбитие строки на слова и фильтрация стоп-слов
    vector<string> SplitIntoWordsNoStop(const string& text) const 
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) 
        {
            if (!IsStopWord(word)) 
            {
                words.push_back(word);
            }
        }
        return words;
    }

    // расчет среднего значения рейтинга документа
    static int ComputeAverageRating(const vector<int>& ratings) 
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

    struct QueryWord 
    {
        string data;
        bool is_minus;
        bool is_stop;
    };

    // возврат слова из запроса и его характеристик: стоп или минус слово
    QueryWord ParseQueryWord(string text) const 
    {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') 
        {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query 
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const 
    {
        Query query;
        for (const string& word : SplitIntoWords(text)) 
        {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) 
            {
                if (query_word.is_minus) 
                {
                    query.minus_words.insert(query_word.data);
                } else 
                {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    // расчет релевантности документа
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const 
    {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) 
            {
                if (document_predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) 
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) 
        {
            if (word_to_document_freqs_.count(word) == 0) 
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) 
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) 
        {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) 
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() 
{
    
} 