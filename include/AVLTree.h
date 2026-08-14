// ============================================================
//  AVLTree.h  —  Template AVL Tree (Array-Indexed Nodes)
//
//  - Template so it works for any type T
//  - Nodes stored in a flat vector (no heap alloc per node)
//      so in-order traversal is cache-friendly
//
//  Requirements:
//    - T must have operator< defined
//
//  Core operations:
//    insert(data)         O(log N) guaranteed (AVL balanced)
//    toVector()           O(N)     in-order, sorted result
//    filter(predicate)    O(N)     in-order, only matching records
//    empty()              O(1)
//    size()               O(1)
// ============================================================
#pragma once

#include <vector>
#include <functional>
using namespace std;

template <typename T>
class AVLTree {
private:
    // ---- Node Structure ----
    // Uses integer indices instead of pointers.
    // All nodes live in one contiguous vector → cache friendly.
    struct Node
    {
        T   data;
        int left   = -1;   // index into nodes[], -1 means null
        int right  = -1;
        int height = 1;

        Node(const T &d) : data(d), left(-1), right(-1), height(1) {}
    };

    vector<Node> nodes;  // all nodes stored here
    int root = -1;

    // ---- Height helpers ----

    int getHeight(int i)
    {
        if (i == -1)
            return 0;
        return nodes[i].height;
    }

    void updateHeight(int i)
    {
        if (i == -1)
            return;
        int lh = getHeight(nodes[i].left);
        int rh = getHeight(nodes[i].right);
        nodes[i].height = 1 + (lh > rh ? lh : rh);
    }

    int getBalance(int i)
    {
        if (i == -1)
            return 0;
        return getHeight(nodes[i].left) - getHeight(nodes[i].right);
    }

    // ---- Rotations ----
    //
    //  rotateRight(y):         rotateLeft(x):
    //      y                       x
    //     / \                     / \
    //    x   C    --->           A   y
    //   / \                         / \
    //  A   B                       B   C

    int rotateRight(int y)
    {
        int x  = nodes[y].left;
        int b  = nodes[x].right;

        // Perform rotation
        nodes[x].right = y;
        nodes[y].left  = b;

        // Update heights (y first — it's now lower)
        updateHeight(y);
        updateHeight(x);

        return x;  // x is the new root of this subtree
    }

    int rotateLeft(int x)
    {
        int y = nodes[x].right;
        int b = nodes[y].left;

        // Perform rotation
        nodes[y].left  = x;
        nodes[x].right = b;

        // Update heights (x first — it's now lower)
        updateHeight(x);
        updateHeight(y);

        return y;  // y is the new root of this subtree
    }

    // ---- Insert ----

    int insertAt(int i, const T &data)
    {
        // Base case: empty spot — create a new node here
        if (i == -1)
        {
            nodes.push_back(Node(data));
            return (int)nodes.size() - 1;
        }

        // Go left or right based on comparison
        if (data < nodes[i].data)
        {
            // Store result first — push_back may reallocate nodes[],
            // so we don't hold a reference across the call
            int newLeft = insertAt(nodes[i].left, data);
            nodes[i].left = newLeft;
        }
        else if (nodes[i].data < data)
        {
            int newRight = insertAt(nodes[i].right, data);
            nodes[i].right = newRight;
        }
        else
        {
            // Duplicate key — just update the data (e.g. same student in 2 companies)
            nodes[i].data = data;
            return i;
        }

        // Update height of this node
        updateHeight(i);

        // Check balance factor
        int bf = getBalance(i);

        // ---- 4 imbalance cases ----

        // Case 1: Left Left  (insert went into left child's left subtree)
        if (bf > 1 && data < nodes[nodes[i].left].data)
            return rotateRight(i);

        // Case 2: Right Right  (insert went into right child's right subtree)
        if (bf < -1 && nodes[nodes[i].right].data < data)
            return rotateLeft(i);

        // Case 3: Left Right  (insert went into left child's right subtree)
        if (bf > 1 && nodes[nodes[i].left].data < data)
        {
            nodes[i].left = rotateLeft(nodes[i].left);
            return rotateRight(i);
        }

        // Case 4: Right Left  (insert went into right child's left subtree)
        if (bf < -1 && data < nodes[nodes[i].right].data)
        {
            nodes[i].right = rotateRight(nodes[i].right);
            return rotateLeft(i);
        }

        // Node is balanced — return as is
        return i;
    }

    // ---- In-order traversal ----
    // Visits left → current → right, producing sorted output

    void inorderAt(int i, vector<T> &out) const
    {
        if (i == -1)
            return;
        inorderAt(nodes[i].left, out);
        out.push_back(nodes[i].data);
        inorderAt(nodes[i].right, out);
    }

    // ---- Filtered traversal ----
    // Same as inorder but only collects records matching pred

    void filterAt(int i, const function<bool(const T &)> &pred, vector<T> &out) const
    {
        if (i == -1)
            return;
        filterAt(nodes[i].left, pred, out);
        if (pred(nodes[i].data))
            out.push_back(nodes[i].data);
        filterAt(nodes[i].right, pred, out);
    }

public:
    // Default constructor — empty tree
    AVLTree() : root(-1) {}

    // Pre-allocate space if you know roughly how many records you'll insert.
    // Avoids vector reallocations during bulk CSV loading.
    void reserve(int n)
    {
        nodes.reserve(n);
    }

    // Insert a record into the tree — O(log N) guaranteed
    void insert(const T &data)
    {
        root = insertAt(root, data);
    }

    // Return all records in sorted order (in-order traversal) — O(N)
    vector<T> toVector() const
    {
        vector<T> result;
        result.reserve(nodes.size());
        inorderAt(root, result);
        return result;
    }

    // Return all records matching pred in sorted order — O(N)
    vector<T> filter(const function<bool(const T &)> &pred) const
    {
        vector<T> result;
        filterAt(root, pred, result);
        return result;
    }

    // True if no records have been inserted
    bool empty() const
    {
        return root == -1;
    }

    // Number of records stored
    int size() const
    {
        return (int)nodes.size();
    }
};
