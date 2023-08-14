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

enum class DocumentStatus 
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator& begin, Iterator& end) : begin_(begin), end_(end) {}

    auto begin()
    {
        return begin_;
    }

    auto end()
    {
        return end_;
    }

    size_t size()
    {
        return distance(begin_, end_);
    }

private:
    Iterator& begin_;
    Iterator& end_;
};

template <typename Iterator>
class Paginator
{
public:
    Paginator(Iterator& begin, Iterator& end, size_t page_size)
    {
        while(distance(begin, end) >= page_size)
        {
            auto current_page_break = next(begin, page_size);
            pages_.push_back({begin, current_page_break});
            begin = current_page_break;
        }
        pages_.push_back({begin, end});
    }

    auto begin()
    {
        return pages_.begin();
    }

    auto end()
    {
        return pages_.end();
    }

    size_t size()
    {
        return pages_.size();
    }

private:
    vector<IteratorRange> pages_;
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
        if(!IsValidDocumentID(document_id))
        {
            throw invalid_argument("Недопустимый id документа!!!"s);
        }
        
        if(IsValidWord(document))
        {
            throw invalid_argument("Текст документа содержит спец. символы!!!"s);
        }
        documents_ids_.push_back(document_id);
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
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance || 
                 ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) && 
                 lhs.rating > rhs.rating);
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
        //if(CheckMinusWord(raw_query)) throw invalid_argument("Недопустимый запрос!!!"s);
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) 
        {
            //if (!IsValidWord(word)) throw invalid_argument("Недопустимый запрос!!!"s);
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
            //if (!IsValidWord(word)) throw invalid_argument("Недопустимый запрос!!!"s);
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
        return documents_ids_.at(index);
    }
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_; // слова, id документов, tf
    map<int, DocumentData> documents_; // id, рейтинг, статус
    vector<int> documents_ids_;

    static bool CheckMinusWord(const string& text) 
    {
       return (text[0] == '-' || text.empty());
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
        if(!IsValidWord(text))
        {
            throw invalid_argument("Запрос содержит спец. символы!!!"s);
        }

        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') 
        {
            is_minus = true;
            text = text.substr(1);

            if(CheckMinusWord(text))
            {
                throw invalid_argument("Ошибка при вводе минус-слов!!!"s);
            }
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

template <typename Container>
auto Paginate(Container& container, size_t page_size)
{
    return Paginator(begin(container), end(container), page_size);
}

int main() 
{
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) 
    {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
} 