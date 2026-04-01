#pragma once

#include <string>
#include <vector>

// Trida SortingTest spusti razeni nad daty a zkontroluje razeni
template <typename SortingAlgorithm>
class SortingTest {
public:
    std::vector<std::string*> sorted_data{};
    SortingAlgorithm sorting_algorithm;

    // konstruktor
    SortingTest(std::vector<std::string>& data, const SortingAlgorithm& algorithm) : sorting_algorithm{algorithm} {
        // nakopiruj data jako pointery - pro rychlejsi razeni
        for (auto& entry : data) {
            sorted_data.push_back(&entry);
        }
    }

    // metoda spusti razeni naddaty
    void run_sort() {
        sorting_algorithm(sorted_data);
    }

    // kontrola vysledku
    bool verify(std::vector<std::string>& data, auto verify_with) {
        if (data.size() != sorted_data.size()) {
            return false;
        }

        // sort data by reference
        std::vector<std::string*> reference;
        // nakopiruj data jako pointery - pro rychlejsi razeni
        for (auto& entry : data) {
            reference.push_back(&entry);
        }

        verify_with(reference);

        // kontrola hodnot
        for (size_t i = 0; i < data.size(); i++) {
            if ((*reference[i]) != (*sorted_data[i])) {
                return false;
            }
        }

        // nenarazili jsme na chybu
        return true;
    }
};
