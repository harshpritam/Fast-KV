#pragma once

#include <iostream>
#include <vector>
#include <stdexcept>
#include <utility> // For std::pair

// Define the color of nodes
enum Color { RED, BLACK };

// Define the structure of the Node
template <typename K, typename V>
struct Node {
    K key;
    V value;
    Color color;
    Node *left, *right, *parent;

    Node(K key, V value) : key(key), value(value), color(RED), left(nullptr), right(nullptr), parent(nullptr) {}
};

template <typename K, typename V>
class RBTree {
private:
    Node<K, V>* root;

    // Helper functions for rotations and balancing
    void leftRotate(Node<K, V>* x);
    void rightRotate(Node<K, V>* y);
    void fixInsert(Node<K, V>* z);
    void transplant(Node<K, V>* u, Node<K, V>* v);
    void inorderTraversal(Node<K, V>* node, std::vector<std::pair<K, V>>& data);
    void deleteNodeHelper(Node<K, V>* node, K key);
    Node<K, V>* minimum(Node<K, V>* node);
    void destroyTree(Node<K, V>* node); // Helper for clear()

public:
    RBTree() : root(nullptr) {}
    ~RBTree() { clear(); } // Destructor to prevent memory leaks

    // Insert key-value pair into the RB tree
    void insert(K key, V value);
    
    // Search for a value by key
    V search(K key);

    // Check if the tree is empty
    bool empty() const { return root == nullptr; }
    
    // Clear the entire tree
    void clear();

    // In-order traversal for printing (debugging)
    void inorderTraversal(Node<K, V>* node);
    void inorderTraversal() { inorderTraversal(root); }
    
    // Get a sorted vector of key-value pairs
    std::vector<std::pair<K, V>> getSortedData();
    
    // Delete a key-value pair from the tree
    void deleteKey(K key);
};


// ----------------------------------------------------------------------------
// --- IMPLEMENTATIONS
// ----------------------------------------------------------------------------

// Left rotation
template <typename K, typename V>
void RBTree<K, V>::leftRotate(Node<K, V>* x) {
    Node<K, V>* y = x->right;
    x->right = y->left;
    if (y->left != nullptr) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == nullptr) {
        root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

// Right rotation
template <typename K, typename V>
void RBTree<K, V>::rightRotate(Node<K, V>* y) {
    Node<K, V>* x = y->left;
    y->left = x->right;
    if (x->right != nullptr) {
        x->right->parent = y;
    }
    x->parent = y->parent;
    if (y->parent == nullptr) {
        root = x;
    } else if (y == y->parent->right) {
        y->parent->right = x;
    } else {
        y->parent->left = x;
    }
    x->right = y;
    y->parent = x;
}

// Fix the Red-Black Tree properties after insertion
template <typename K, typename V>
void RBTree<K, V>::fixInsert(Node<K, V>* z) {
    while (z->parent != nullptr && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            Node<K, V>* y = z->parent->parent->right;
            if (y != nullptr && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    leftRotate(z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                rightRotate(z->parent->parent);
            }
        } else {
            Node<K, V>* y = z->parent->parent->left;
            if (y != nullptr && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    rightRotate(z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                leftRotate(z->parent->parent);
            }
        }
    }
    root->color = BLACK;
}

// Insert key-value pair into the RB tree
template <typename K, typename V>
void RBTree<K, V>::insert(K key, V value) {
    Node<K, V>* y = nullptr;
    Node<K, V>* x = root;

    while (x != nullptr) {
        y = x;
        if (key < x->key) {
            x = x->left;
        } else if (key > x->key) {
            x = x->right;
        } else {
            // Key already exists, update the value
            x->value = value;
            return;
        }
    }

    Node<K, V>* z = new Node<K, V>(key, value);
    z->parent = y;
    if (y == nullptr) {
        root = z;
    } else if (z->key < y->key) {
        y->left = z;
    } else {
        y->right = z;
    }

    fixInsert(z);
}

// Search for a value by key
template <typename K, typename V>
V RBTree<K, V>::search(K key) {
    Node<K, V>* node = root;
    while (node != nullptr) {
        if (key == node->key) {
            return node->value;
        } else if (key < node->key) {
            node = node->left;
        } else {
            node = node->right;
        }
    }
    throw std::runtime_error("Key not found");
}

// In-order traversal (for visualization or debugging)
template <typename K, typename V>
void RBTree<K, V>::inorderTraversal(Node<K, V>* node) {
    if (node != nullptr) {
        inorderTraversal(node->left);
        std::cout << "Key: " << node->key << ", Value: " << node->value << ", Color: " 
             << (node->color == RED ? "RED" : "BLACK") << std::endl;
        inorderTraversal(node->right);
    }
}

// In-order traversal to populate a vector
template <typename K, typename V>
void RBTree<K, V>::inorderTraversal(Node<K, V>* node, std::vector<std::pair<K, V>>& data) {
    if (node != nullptr) {
        inorderTraversal(node->left, data);
        data.push_back(std::make_pair(node->key, node->value));
        inorderTraversal(node->right, data);
    }
}

// Get sorted data vector
template <typename K, typename V>
std::vector<std::pair<K, V>> RBTree<K, V>::getSortedData() {
    std::vector<std::pair<K, V>> sortedData;
    inorderTraversal(root, sortedData);
    return sortedData;
}

// Delete a key-value pair from the tree
template <typename K, typename V>
void RBTree<K, V>::deleteKey(K key) {
    deleteNodeHelper(root, key);
}

// Helper function for deleting a node
template <typename K, typename V>
void RBTree<K, V>::deleteNodeHelper(Node<K, V>* node, K key) {
    // This is a simplified delete and does not include the RB-tree fixup
    // A full implementation is much more complex
    Node<K, V>* z = node;
    while (z != nullptr) {
        if (z->key == key) {
            break;
        }
        z = (key < z->key) ? z->left : z->right;
    }

    if (z == nullptr) {
        return; // Key not found
    }

    Node<K, V>* y = z;
    // ... A full RB-Tree delete fixup is required here
    // This simplified version will break RB properties.
    // For now, this just does a basic BST delete.
    if (z->left == nullptr) {
        transplant(z, z->right);
    } else if (z->right == nullptr) {
        transplant(z, z->left);
    } else {
        y = minimum(z->right);
        if (y->parent != z) {
            transplant(y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        transplant(z, y);
        y->left = z->left;
        y->left->parent = y;
    }
    delete z;
}

// Find the minimum node in a subtree
template <typename K, typename V>
Node<K, V>* RBTree<K, V>::minimum(Node<K, V>* node) {
    while (node->left != nullptr) {
        node = node->left;
    }
    return node;
}

// Helper to replace one subtree with another
template <typename K, typename V>
void RBTree<K, V>::transplant(Node<K, V>* u, Node<K, V>* v) {
    if (u->parent == nullptr) {
        root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != nullptr) {
        v->parent = u->parent;
    }
}

// Helper to recursively delete all nodes
template <typename K, typename V>
void RBTree<K, V>::destroyTree(Node<K, V>* node) {
    if (node != nullptr) {
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
    }
}

// Clears the entire tree to prevent memory leaks
template <typename K, typename V>
void RBTree<K, V>::clear() {
    destroyTree(root);
    root = nullptr;
}