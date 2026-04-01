#include "vector_sum.h"
#include "data_generator.h"
#include "executor.h"
#include "TextTable.h"

#include <string>
#include <ios>
#include <iomanip>
#include <chrono>

// Konverze desetinneho cisla na string s fixni presnosti
std::string to_string(double x, int precision) {
    std::stringstream s;
    s << std::fixed << std::setprecision(precision) << x;
    return s.str();
}

// Konverze casu testu na string
std::string to_string(std::optional<std::chrono::nanoseconds> test_time) {
    if (!test_time) {
        return "!!!"; // nespravny vysledek
    }

    auto ns = test_time->count();
    if (ns >= 1'000'000'000) return to_string((double)ns / 1e9, 2) + " s";
    else if (ns >= 1'000'000) return to_string((double)ns / 1e6, 2) + " ms";
    else if (ns >= 1'000) return to_string((double)ns / 1e3, 2) + " us";
    else return std::to_string(ns) + " ns";
}

// Naplneni radku tabulky vysledku
void add_table_row(const std::string& label, const Results& results, TextTable& table) {
    table.add(label);
    table.add(to_string(results.time_ref));
    table.add(to_string(results.time_per_vector));
    table.add(to_string(results.time_shuffle));
    table.add(to_string(results.time_omp_dynamic));
    table.add(to_string(results.time_omp_static));
    table.endOfRow();
}

// Prvni sada dat, ktera obsahuje velice malo hodne dlouhych vektoru
// Tato sada je vhodna pro paralelizaci na urovni vektoru. Tento zpusob paralelizace by mel byt v tomto pripade nejrychlejsi
void test_dataset_0(TextTable& table) {
    //vytvarime 3 vektory, ktere maji kazdy 250'000'000 pseudonahodne generovanych cisel
    InputVectors data(3, std::vector<int8_t>(250'000'000));
    //spravne reseni
    OutputVector solution(data.size());
    //nagenerujeme data a ulozime si spravne reseni
    data_generator::generate_data(solution, data);

    // zavolame metody, ktere jste naimplementovali a pridame radek do tabulky
    auto results = executor::execute_methods(solution, data);
    add_table_row("Malo hodne dlouhych vektoru", results, table);
}

// Druha sada dat, kde delka vektoru je generovana z normalni distribuce podle parametru "mean" a "sigma".
// Nejrychlejsim resenim by melo byt pouziti dynamickeho schedulingu. Alternativne si vyzkousite data pred
// pouzitim statickeho schedulingu promichat. Zkuste se zamyslet proc bychom chteli v nekterych pripadech
// pridavat dalsi rezii v podobe michani
void test_dataset_1(TextTable& table) {
    // Delky vektoru vygenerujeme ze dvou normalnich rozdeleni. Vetsina dat pochazi z rozdeleni s malou
    // stredni hodnotou (a dane vektory tedy zpracujeme velmi rychle). Cast dat ale trva velmi dlouho
    // (jsou generovana z rozdeleni z vysokou stredni hodnotou), a budou tedy narocne na zpracovani.
    // V nasem pripade se data narocna na zpracovani nachazi na zacatku datove sady - ale v obecnosti
    // mohou byt kdekoliv.

    std::array<size_t, 8192> lengths{};

    // 1/8th of vectors is very long
    auto large_data_count = lengths.size() / 8;
    auto split_it = lengths.begin() + (ptrdiff_t)large_data_count;

    data_generator::generate_vector_sizes(lengths.begin(), split_it, 500'000, 300'000);
    data_generator::generate_vector_sizes(split_it, lengths.end(), 5, 3);

    InputVectors data;
    for (auto j : lengths) {
        std::vector<int8_t> vec(j);
        data.push_back(vec);
    }
    OutputVector solution(data.size());
    data_generator::generate_data(solution, data);

    // zavolame metody, ktere jste naimplementovali a pridame radek do tabulky
    auto results = executor::execute_methods(solution, data);
    add_table_row("Delky s velkym rozptylem", results, table);
}

// Treti sada dat, ktera obsahuje velky pocet vektoru male konstatntni velikost.
// Vase reseni s redukci by melo byt napsane tak, aby bylo srovnatelne se sekvencim resenim. To znamena, ze
// se vypocet v pripade redukce nebude vykonavat paralelne.
// Proc v tomto pripade muze byt dynamicke rozvrhovani horsi nez staticke?
void test_dataset_2(TextTable& table) {
    InputVectors data(10000000, std::vector<int8_t>(2));
    OutputVector solution(10000000);
    data_generator::generate_data(solution, data);

    // zavolame metody, ktere jste naimplementovali a pridame radek do tabulky
    auto results = executor::execute_methods(solution, data);
    add_table_row("Hodne kratkych vektoru", results, table);
}

// Ctvrta sada obsahuje data, ktera jsou nevhodna k paralelizaci. Vektoru je malo a navic jsou
// velmi kratke. V tomto pripade by mela mit na vrch sekvencni verze, ktera nema zadnou rezii
// na paralelizaci.
void test_dataset_3(TextTable& table) {
    InputVectors data(10, std::vector<int8_t>(10));
    OutputVector solution(data.size());
    data_generator::generate_data(solution, data);

    auto results = executor::execute_methods(solution, data);
    add_table_row("Data nevhodna k paralelizaci", results, table);
}

int main() {
    TextTable table{};

    // zahlavi tabulky
    table.add("");
    table.add("Sekvencni");
    table.add("P. na urovni vektoru");
    table.add("Michani");
    table.add("Dynamicke rozvrhovani");
    table.add("Staticke rozvrhovani");
    table.endOfRow();

    // pro blizsi informace se podivejte na samotne metody s komentarem
    test_dataset_0(table);
    test_dataset_1(table);
    test_dataset_2(table);
    test_dataset_3(table);

    // vytisknuti tabulky s vysledky
    table.setAlignment(0, TextTable::Alignment::RIGHT);
    std::cout << table;

    return 0;
}
