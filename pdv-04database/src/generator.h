#pragma once

#include <cstdint>
#include <functional>
#include <vector>

// vsechny testy pouzite v teto uloze pouzivaji nasledujici generator dat, ktery generuje data (tabulku a dotaz nad ni)
// na zaklade zadanych parametru. V uloze generujeme 2 typy dotazu, ktere se jednou maji evaluovat jako pravdive a jindy
// jako nepravdive. V prvnim pripade je dotaz slozeny s predikatu, ktere jsou vsechny spojeny disjunkci. V druhem pripade
// jsou predikaty spojeny konjunkci. Aby dany dotaz byl vzhledem k dane tabulce splnen, musim byt vzdy pro konjunkci
// vsechny predikaty pravdive. U konjunkce staci, aby byl splnen nejaky.

// vygeneruje tabulku a dotaz v podobe konjunkce predikatu
// expected - zda ma byt dotaz a data vygenerovana tak, ze dotaz je nad danymi daty pravdivy
auto generate_all(bool expected) -> std::pair<std::vector<uint32_t>, std::vector<std::function<bool(uint32_t)>>>;

// vygeneruje tabulku a dotaz v podobe disjunkce predikatu
// expected - zda ma byt dotaz a data vygenerovana tak, ze dotaz je nad danymi daty pravdivy
auto generate_any(bool expected) -> std::pair<std::vector<uint32_t>, std::vector<std::function<bool(uint32_t)>>>;
