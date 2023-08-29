#pragma once

#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator& begin, Iterator& end) : begin_(begin), end_(end) {}

    auto begin() const
    {
        return begin_;
    }

    auto end() const
    {
        return end_;
    }

    size_t size() const
    {
        return distance(begin_, end_);
    }

private:
    Iterator begin_;
    Iterator end_;
};

template<typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator>& it)
{
    for (const auto& elem : it)
    {
        out << elem;
    }
    return out;
}

template <typename Iterator>
class Paginator
{
public:
    Paginator(Iterator begin, Iterator end, size_t page_size)
    {
        Iterator current = begin;
        while (current != end)
        {
            Iterator current_page_break = current;
            advance(current_page_break, min(page_size, size_t(distance(current, end))));
            pages_.emplace_back(current, current_page_break);
            current = current_page_break;
        }
    }

    auto begin() const
    {
        return pages_.begin();
    }

    auto end() const
    {
        return pages_.end();
    }

    size_t size() const
    {
        return pages_.size();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
};

template<typename Iterator>
std::ostream& operator<<(std::ostream& out, std::vector<IteratorRange<Iterator>> iter_vec)
{
    for (const auto elem : iter_vec)
    {
        out << elem;
    }
    out << endl;
    return out;
}

template <typename Container>
auto Paginate(Container& container, size_t page_size)
{
    return Paginator(begin(container), end(container), page_size);
}