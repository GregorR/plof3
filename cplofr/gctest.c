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
    node **root = (node **) pgcNewRoot(3*sizeof(node*), 3);

    /* must have a node to start with */
    MGC_NEW(root, node);
    root[0]->left = *root;
    root[0]->right = *root;

    for (i = 0; i < 10000000; i++) {
        /* find a random node */
        root[1] = random_node(*root, 10);

        /* associate it with a new node */
        printf("A %p\n", root[1]);
        MGC_NEW(root+2, node);
        printf("B %p\n", root[1]);
        if (rand() % 2 == 0) {
            root[1]->left = root[2];
        } else {
            root[1]->right = root[2];
        }

        /* then connect its left and right to random nodes */
        root[2]->left = root[2]->right = root[2];
        root[2]->left = random_node(*root, 10);
        root[2]->right = random_node(*root, 10);
    }

    return 0;
}
