#pragma once

#include <cstdint>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include "../pdv_lib/benchmark.hpp"
#include "SpinMutex.h"

class ConcurrentLinkedList {
public:
    class Node {
    public:
        uint64_t value;
        Node* next = nullptr;
        /** Spinlock pro aktualni uzel. */
        SpinMutex mutex;

        explicit Node(uint64_t value) : value(value) {}
    };

    Node head{0};

    // TODO: Naimplementujte vlaknove-bezpecne vkladani do konkurentniho spojoveho seznamu. Pro popis struktury
    //  spojoveho seznamu se podivejte do souboru `LinkedList.h`, kde naleznete i implementaci vkladani do spojoveho
    //  seznamu, ktera neni vlaknove bezpecna (tj., muze ji provadet pouze jedno vlakno). Vsimnete si, ze trida `Node`
    //  reprezentujici uzel spojoveho seznamu obsahuje svuj vlastni mutex `m`.
    void insert(uint64_t value) {
        Node* node = new Node(value);

        auto current = &head;
        while (true) {
            auto lock = std::unique_lock(current->mutex);
            if (current->next == nullptr || current->next->value > value) {
                // vkladam prvek
                node->next = current->next;
                current->next = node;
                break;
            } else {
                // pokracuju dal
                current = current->next;
            }
        }
    }

    // TODO: Jako bonus muzete zkusit naimplementovat i mazani
    bool remove(uint64_t value) {
        Node* prev = &head;
        Node* current = head->next;
        while (current != nullptr) {
            auto lock = std::unique_lock(current->mutex);
            if (current->value == value) {
                break;
            }
            else {

            }
        }

    }
};
