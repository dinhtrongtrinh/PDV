#include "query.h"
#include "atomic"

// NOTE: Varování 'warning: `cancel for` inside `nowait` for construct' není třeba řešit, jedná se pravdepodobně
//  o chybu v interních optimalizacích compileru GCC, viz
//  https://stackoverflow.com/questions/30275513/gcc-5-1-warns-cancel-construct-within-parallel-for-construct

bool is_satisfied_for_all(const std::vector<Predicate<uint32_t>>& predicates, const std::vector<uint32_t>& data) {
    // 1. Globální příznak, že alespoň jeden predikát SELHAL
    std::atomic<bool> any_failed{false};

    // 2. Paralelizujeme VNĚJŠÍ cyklus (každé vlákno dostane svůj predikát)
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < predicates.size(); ++i) {

        // Pokud už nějaké jiné vlákno zjistilo selhání, tohle vlákno už nic dělat nemusí

        const auto& predicate = predicates[i];
        bool found_match = false;

        // 3. Vnitřní cyklus prochází data (sekvenčně v rámci jednoho vlákna)
        for (const auto& item : data) {
            // Průběžná kontrola: pokud to už někdo jiný pokazil, ukonči hledání v datech

            if (predicate(item)) {
                found_match = true; // Našli jsme aspoň jeden záznam!
                break;              // Pro tento predikát máme hotovo
            }
        }
        // 4. Pokud jsme po prohledání všech dat nenašli shodu pro TENTO predikát:
        #pragma omp cancellation point for
        if (!found_match) {
            any_failed = true; // Právě jsme odsoudili celý dotaz k výsledku false
            #pragma omp cancel for
        }
    }

    // Pokud nic neselhalo, vrátíme true
    return !any_failed;
}

bool is_satisfied_for_any(const std::vector<Predicate<uint32_t>>& predicates, const std::vector<uint32_t>& data) {
    std::atomic<bool> is_true{false};

    #pragma omp parallel for schedule(dynamic, 100)
    for (size_t i = 0; i < data.size(); ++i) {
        // "Ruční brzda": Pokud už někdo jiný našel shodu, tohle vlákno už nic nedělá
        if (is_true) continue;

        const auto& data_item = data[i];

        for (const auto& predicate : predicates) {
            // Kontrola i uvnitř vnitřního cyklu (pro případ, že predikátů je hodně)
            if (is_true) break;

            if (predicate(data_item)) {
                is_true = true; // Našli jsme aspoň jednu jehlu v kupce sena!
                break;
            }
        }
    }
    return is_true;
}
