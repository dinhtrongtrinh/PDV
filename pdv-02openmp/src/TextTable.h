#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <iomanip>
#include <vector>
#include <string>

// Prejato a upraveno z https://github.com/haarcuba/cpp-text-table. Slouzi k vypisu tabulky.
class TextTable {

public:
    enum class Alignment {
        LEFT, RIGHT
    };
    typedef std::vector<std::string> Row;

    TextTable() = default;

    void setAlignment(unsigned i, Alignment alignment) {
        _alignment[i] = alignment;
    }

    Alignment alignment(unsigned i) const { return _alignment[i]; }

    void add(std::string const &content) {
        _current.push_back(content);
    }

    void endOfRow() {
        _rows.push_back(_current);
        _current.assign(0, "");
    }

    template<typename Iterator>
    void addRow(Iterator begin, Iterator end) {
        for (auto i = begin; i != end; ++i) {
            add(*i);
        }
        endOfRow();
    }

    template<typename Container>
    void addRow(Container const &container) {
        addRow(container.begin(), container.end());
    }

    std::vector<Row> const &rows() const {
        return _rows;
    }

    void setup() const {
        determineWidths();
        setupAlignment();
    }

    size_t width(size_t i) const { return _width[i]; }

private:
    Row _current;
    std::vector<Row> _rows;
    std::vector<size_t> mutable _width;
    std::map<unsigned, Alignment> mutable _alignment;

    unsigned columns() const {
        return static_cast<unsigned>(_rows[0].size());
    }

    void determineWidths() const {
        _width.assign(columns(), 0);
        for (const auto & row : _rows) {
            for (unsigned i = 0; i < row.size(); ++i) {
                _width[i] = _width[i] > row[i].size() ? _width[i] : static_cast<unsigned>(row[i].size());
            }
        }
    }

    void setupAlignment() const {
        for (unsigned i = 0; i < columns(); ++i) {
            if (_alignment.find(i) == _alignment.end()) {
                _alignment[i] = Alignment::LEFT;
            }
        }
    }
};

std::ostream &operator<<(std::ostream &stream, TextTable const &table) {
    table.setup();
    for (auto rowIterator = table.rows().begin(); rowIterator != table.rows().end(); ++rowIterator) {
        TextTable::Row const &row = *rowIterator;
        for (size_t i = 0; i < row.size(); ++i) {
            auto alignment = table.alignment(i) == TextTable::Alignment::LEFT ? std::left : std::right;
            stream << std::setw((int)table.width(i)) << alignment << row[i];
            stream << "  ";
        }
        stream << "\n";
    }

    return stream;
}
