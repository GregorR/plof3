#include <stdio.h>
#include <stdlib.h>

#include <gc/gc.h>

#include "gc.h"

PGC_STRUCT(node, 2) {
    node *left, *right;
};

#ifdef REAL_GC
#define MGC_NEW PGC_NEW
#else
#define MGC_NEW(into, type) (*(into) = (type *) GC_malloc(sizeof(type)))
#endif

node *random_node(node *from, int c)
{
    if (c == 0) return from;

    if (rand() % 2 == 0) {
        return random_node(from->left, c-1);
    } else {
        return random_node(from->right, c-1);
    }
}

int main()
{
    int i;
    pgcInit();
    node **root = (node **) pgcNewRoot(sizeof(void*), 1);

    /* must have a node to start with */
    MGC_NEW(root, node);
    (*root)->left = *root;
    (*root)->right = *root;

    for (i = 0; i < 10000000; i++) {
        node *rn, *nn;

        /* find a random node */
        rn = random_node(*root, 10);

        /* associate it with a new node */
        if (rand() % 2 == 0) {
            nn = MGC_NEW(&(rn->left), node);
        } else {
            nn = MGC_NEW(&(rn->right), node);
        }

        /* then connect its left and right to random nodes */
        nn->left = nn->right = nn;
        nn->left = random_node(*root, 10);
        nn->right = random_node(*root, 10);
    }

    return 0;
}
