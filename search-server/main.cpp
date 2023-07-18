#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include<math.h>

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

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    // запись стоп-слов из ввода
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    // добавление нового документа
    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size(); // расчет term frequency
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count; 
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    // возвращает MAX_RESULT_DOCUMENT_COUNT документов с наибольшей релевантностью/рейтингом
    template <typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper key_mapper) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance || ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) && lhs.rating > rhs.rating);
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    // возвращает MAX_RESULT_DOCUMENT_COUNT документов с наибольшей релевантностью/рейтингом
    // вариант функции с возможностью указать статус документа
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus document_status = DocumentStatus::ACTUAL) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, [document_status](int document_id, DocumentStatus status, int rating) { return status == document_status; });

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance || ((std::abs(lhs.relevance - rhs.relevance) < EPSILON) && lhs.rating > rhs.rating);
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    // количество документов в системе
    int GetDocumentCount() const
    {
        return static_cast<int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const 
    {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_; // слова, id документов, tf
    map<int, DocumentData> documents_; // id, рейтинг, статус

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    // разбитие строки на слова и фильтрация стоп-слов
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    // расчет среднего значения рейтинга документа
    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    // возврат слова из запроса и его характеристик: стоп или минус слово
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
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
    template <typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query, KeyMapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (key_mapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// макросы для тестирования
template <typename Key, typename Value>
ostream& operator<<(ostream& out, pair<Key, Value> pr)
{
    return out << pr.first << ": "s << pr.second;
}

template <typename C>
ostream& Print(ostream& out, C& container)
{
    bool is_first = true;
    for(const auto& elem : container)
    {
        if(is_first)
        {
            out << elem;
            is_first = false;
            continue;
        }

        out << ", "s << elem;
    }

    return out;
}

template <typename E>
ostream& operator<<(ostream& out, vector<E> container)
{
    out << '[';
    Print(out, container);
    out << ']';

    return out;
}

template <typename E>
ostream& operator<<(ostream& out, set<E> container)
{
    out << '{';
    Print(out, container);
    out << '}';

    return out;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& out, map<Key, Value> container)
{
    out << '{';
    Print(out, container);
    out << '}';

    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// макрос для запуска тестов
template <typename F>
void RunTestImpl(const F& fnc, const string& fnc_str) 
{
    fnc();
    cerr <<  fnc_str  << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)


// -------- Начало модульных тестов поисковой системы ----------

SearchServer GetTestSearchServer(int doc_id, string content, vector<int> ratings) {
    SearchServer server;    

    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    return server;
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddDocument() {
    SearchServer server = GetTestSearchServer(42, "cat in the city"s, {1, 2, 3});
    ASSERT(!server.FindTopDocuments("cat in the bar"s).empty());
    
    server.AddDocument(43, "in the", DocumentStatus::ACTUAL, {2, 3, 2});

    ASSERT(server.FindTopDocuments("in the").empty());
}

void TestStopWords() {
    const SearchServer server = GetTestSearchServer(42, "cat in the city"s, {1, 2, 3});
    ASSERT(server.FindTopDocuments("in the"s).empty());
    ASSERT(server.FindTopDocuments("pig in the"s).empty());
    ASSERT(!server.FindTopDocuments("a dog found in the cat").empty());
}

void TestMinusWords() {
    SearchServer search_server;
    search_server.AddDocument(0, "white cat and fashionable collar"s, DocumentStatus::ACTUAL, {8, -3});
    vector<Document> results = search_server.FindTopDocuments("cat -white"s);
    ASSERT(results.empty());
}

void TestMatching() {
    int doc_id = 42;
    SearchServer server = GetTestSearchServer(doc_id, "cat in the city"s, {1, 2, 3});
    
    auto [ matched_words, doc_status ] = server.MatchDocument("cat in the city"s, doc_id);
    
    ASSERT_EQUAL(matched_words.size(), 2u);

    tie(matched_words, doc_status) = server.MatchDocument("-cat in the bar", 42);
    ASSERT(matched_words.empty());

    doc_id = 33;

    server.AddDocument(doc_id, "hello world cat bat cat", DocumentStatus::ACTUAL, {1, 1, 1, 1, 4});

    tie(matched_words, doc_status) = server.MatchDocument("cat"s, doc_id);
    ASSERT_EQUAL(matched_words.size(), 1u);
}

void TestRelevanceSorting() {
    SearchServer server = GetTestSearchServer(42, "cat in the city"s, {1, 2, 3});
    server.AddDocument(43,"dog in the city"s,DocumentStatus::ACTUAL,{1, 2, 2});
    server.AddDocument(44,"pig in the bar"s,DocumentStatus::ACTUAL,{3, 2, 5});
    auto documents = server.FindTopDocuments("cat in the city"s);

    auto documents_sorted(documents);
    
    ASSERT_EQUAL(documents[0].id, 42u);
    ASSERT_EQUAL(documents[1].id, 43u);

    server.AddDocument(45,"cat in the city"s, DocumentStatus::ACTUAL,{2, 2, 5});
    
    documents = server.FindTopDocuments("cat in the city"s);
    
    ASSERT_EQUAL(documents[0].id, 45u);
    ASSERT_EQUAL(documents[1].id, 42u);
    ASSERT_EQUAL(documents[2].id, 43u);
}

void TestRating() {
    SearchServer server = GetTestSearchServer(42, "cat in the city"s, {1, 2, 3});
    auto document = server.FindTopDocuments("cat"s)[0];

    ASSERT_EQUAL(document.rating, 2u);

    server.AddDocument(43,"pig in the subway"s,DocumentStatus::ACTUAL,{1, 4, 5, 6, 4});

    document = server.FindTopDocuments("pig"s)[0];
    ASSERT_EQUAL(document.rating, 4u);
}

void TestFilters() {
    SearchServer server = GetTestSearchServer(42, "cat in the city"s, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1, 2, 2});
    server.AddDocument(44, "pig in the bar"s, DocumentStatus::BANNED, {3, 2, 5});
    auto documents = server.FindTopDocuments("Cat"s, 
            [](int document_id, DocumentStatus status, int rating) -> bool 
            {
                return document_id % 2 == 0;
            }
        );
    for (const auto& document : documents) {
        ASSERT_EQUAL(document.id % 2, 0u);
    }

    documents = server.FindTopDocuments("pig"s, 
        [](int document_id, DocumentStatus status, int rating) -> bool 
        {
            return status == DocumentStatus::BANNED;
        });

    ASSERT_EQUAL(documents.size(), 1u);
}

void TestRelevance() {
    SearchServer server = GetTestSearchServer(42, "cat in the city"s, {1, 2, 3});
    server.AddDocument(43, "dog in the city"s, DocumentStatus::ACTUAL, {1, 2, 2});
    server.AddDocument(44, "pig in the bar"s, DocumentStatus::BANNED, {3, 2, 5});
    
    const auto documents = server.FindTopDocuments("Cat"s); 
    const vector<double> relevance_values = {log(3), 0, 0};
    const double EPSILON = 10e-6;

    for (int i = 0; i < documents.size(); ++i) {
        ASSERT(abs(relevance_values[i] - documents[i].relevance) < EPSILON);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() 
{
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestStopWords);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatching);
    RUN_TEST(TestRelevanceSorting);
    RUN_TEST(TestRating);
    RUN_TEST(TestFilters);
    RUN_TEST(TestRelevance);
}
// --------- Окончание модульных тестов поисковой системы -----------

int main() 
{
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}