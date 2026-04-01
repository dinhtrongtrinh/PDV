#pragma once

#include <cstdint>

/**
 * Sekvencni implementace serazeneho spojoveho seznamu, ktery zvlada pouze operaci 'insert'.
 * Z teto implementace muzete vychazet pri implementaci Vasi konkurentni verze.
 */
class SequentialLinkedList {
public:
    /** Trida reprezentujici jeden uzel spojoveho seznamu. */
    class Node {
    public:
        /** Hodnota prvku, ktery je reprezentovany danym uzlem. */
        uint64_t value;
        /** Ukazatel na nasledujici uzel seznamu. */
        Node* next = nullptr;

        explicit Node(uint64_t value) : value(value) {}
    };

    // Aby se nam se spojovym seznamem pracovalo lepe (a nemuseli jsme rozlisovat
    // mezi prazdnym a neprazdnym seznamem v nasich metodach), uvazujeme, ze seznam
    // vzdy obsahuje alespon jeden prvek. Tento prvek reprezentuje "nulty" prvek
    // seznamu, ktery bude v seznamu pritomny vzdy.
    Node head{0};

    /** Vlozi prvek `value` na spravne misto v seznamu. */
    void insert(uint64_t value) {
        Node* node = new Node(value); // Novy uzel seznamu, ktery budeme vkladat

        // Nulty prvek je vzdy v seznamu pritomny a nikdy ho nebudeme muset
        // "posunout" dale v seznamu. Vzdy budeme vkladat az za nej.
        Node* current = &head;

        // Toto neni nejelegantnejsi implementace vkladani do seznamu, ale doufame,
        // ze Vam poslouzi lepe pri psani konkurentni verze:
        while (true) {
            Node* next = current->next; // Naslednik aktualniho prvk
            if (next == nullptr || next->value > value) {
                // Novy uzel obsahujici hodnotu 'value' chceme vlozit za prvek `current`,
                // pokud nastane jedna z techto moznosti:
                //  1) Dosli jsme na konec seznamu (tj. vsechny prvky jsou mensi nez `value`).
                //  2) Nasledujici prvek `next` ma hodnotu vyssi nez `value`
                //     (a soucasne vsechny predchazejici prvky maji hodnotu nizsi)

                // Prehodime pointery a zaclenime novy `node` mezi prvky `current` a `next`:
                node->next = next;
                current->next = node;

                // A skoncime provadeni funkce
                return;
            } else {
                // V opacnem pripade bude spravne misto pro vlozeni lezet dale v seznamu a musime ho najit.
                // Posuneme se na dalsi prvek a vratime se na zacatek while smycky.
                current = next;
            }
        }
    }

    bool remove(uint64_t value) {
        Node* current = &head;
        while (true) {
            Node* next = current->next;
            if (next == nullptr) {
                return false;
            }

            if (next->value == value) {
                current->next = next->next;
                delete next;
                return true;
            } else {
                current = next;
            }
        }
    }
};
