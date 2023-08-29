#include "document.h"

using namespace std;

ostream& operator<<(ostream& os, const Document document)
{
    os << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating
    << " }"s;
    
    return os;
}