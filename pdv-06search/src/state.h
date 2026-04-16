#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>

class state;
/// Zkraceny alias pro casto pouzivany shared pointer na stav.
using state_ptr = std::shared_ptr<const state>;

/**
 * Abstraktni trida reprezentujici stav problemu. Jednotlive problemy imple-
 * mentuji tuto tridu.
 *
 * POZOR! V prubehu prohledavani muze pro jeden stav problemu existovat vice
 * instanci tridy state.
 */
class state {
private:
    const uint64_t _cost;
    const state_ptr _previous_state;

public:

    /**
     * Metoda 'next_states()' vraci seznam nasledniku aktualniho stavu problemu.
     * Nasledniky stavu si muzete predstavit jako sousedni vrcholy v prohleda-
     * vanem grafu.
     *
     * @return Seznam nasledniku aktualniho stavu jako vektor ukazatelu
     */
    [[nodiscard]] virtual std::vector<state_ptr> next_states() const = 0;

    /**
     * Metoda pro zjisteni, zda je aktualni stav cilovym stavem.
     *
     * POZOR! V nekterych problemech muze byt vice cilovych stavu (napr., pri
     * splnovani logicke funkce v problemu 'sat_domain' muze existovat vice
     * modelu.
     *
     * @return 'true' pokud je aktualni stav cilovym
     */
    [[nodiscard]] virtual bool goal() const = 0;

    /**
     * Metoda pro ziskani celkove ceny cesty vedouci do aktualniho stavu.
     * Muzete predpokladat, ze cena kazde hrany je vzdy >= 1.
     *
     * @return Cena cesty vedouci do aktualniho stavu
     */
    [[nodiscard]] uint64_t total_cost() const {
        return _cost;
    }

    /**
     * Tuto metodu muzete pouzit pro ziskani identifikatoru aktualniho stavu.
     * Tento identifikator bude pro dany stav unikatni (bez ohledu na cestu,
     * kterou jste pro jeho dosazeni pouzili).
     *
     * POZOR! Pokud prekrocite povolene rozsahy parametrizace domeny, garanci
     * unikatnosti identifikatoru ztracite!
     *
     * @return Identifikator aktualniho stavu.
     */
    [[nodiscard]] virtual uint64_t id() const = 0;

    /**
     * V nekterych pripadech se Vam muze hodit zjistit, jakou cestou byl dany
     * stav dosazeny (napr., pro jednoduchou implementaci closed-listu). Tato
     * metoda vraci predchudce aktualniho stavu na pouzite ceste (pokud exis-
     * tuje). V opacnem pripade je pointer nastaven na 'nullptr'.
     *
     * Priklad pouziti:
     *   std::vector<state_ptr> path;
     *   state_ptr state = ...;
     *
     *   while(state) {
     *     path.push_back(state);
     *     state = state->previous_state();
     *   }
     *
     *   std::reverse(path.begin(), path.end());
     *
     *
     * @return Ukazatel na predchazejici stav na ceste pouzite pro dosazeni
     *         aktualniho stavu
     */
    [[nodiscard]] state_ptr previous_state() const {
        return _previous_state;
    }

    /**
     * Metoda pro ziskani textove reprezentace aktualniho stavu. Tuto metodu
     * muzete vyuzit napriklad pri ladeni Vaseho kodu.
     *
     * @return Textova reprezentace aktualniho stavu.
     */
    [[nodiscard]] virtual std::string to_string() const = 0;


protected:
    state(state_ptr previous, uint64_t cost) : _cost(cost), _previous_state(std::move(previous)) {}

    virtual ~state() = default;
};

template<class T>
concept domain = requires(const T& t, std::ostream& os) {
    { T() };
    { t.root() } -> std::convertible_to<state_ptr>;
    { os << t } -> std::convertible_to<std::ostream&>;
};
