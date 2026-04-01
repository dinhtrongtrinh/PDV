#include "bst_tree.h"

void bst_tree::insert(int64_t data) {
    node* new_node = new node(data);
    node* expected = nullptr;

    if(root.compare_exchange_strong(expected, new_node))
    {
        return;
    }
    // TODO: Naimplementujte zde vlaknove-bezpecne vlozeni do binarniho vyhledavaciho stromu
    node* curr = root;
    while (true) {
        node* left = curr->left.load();
        node* right = curr->right.load();
        if (curr->data > data) {
            // tady pridavam
            if (curr->left == nullptr) {
                new_node->left.store(left);
                if (curr->left.compare_exchange_strong(left, new_node)) {
                    return;
                }
            }
            else {
                curr = curr->left.load();
            }
        }
        else if (curr->data <= data) {

            if (curr->right == nullptr) {
                new_node->right.store(right);
                if (curr->right.compare_exchange_strong(right, new_node)) {
                    return;
                }
            }
            else {
                curr = curr->right.load();
            }
        }
    }

}

// Rekurzivni funkce pro pruchod stromu a dealokaci pameti prirazene jednotlivym uzlum
void delete_node(bst_tree::node* node) {
    if (node == nullptr) {
        return;
    }

    delete_node(node->left);
    delete_node(node->right);
    delete node;
}

bst_tree::~bst_tree() {
    delete_node(root);
}
