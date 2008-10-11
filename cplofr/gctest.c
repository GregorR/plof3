#include <stdio.h>
#include <stdlib.h>

#include "gc.h"

PGC_STRUCT(tree, 2) {
    tree *left, *right;
    int val;
};

#ifdef REAL_GC
#define MGC_NEW PGC_NEW
#define MGC_FREE(arg)
#else
#define MGC_NEW(into, type) (*into = (type *) malloc(sizeof(type)))
#define MGC_FREE(arg) free(arg)
#endif

/* tree functions */
void tree_insert(tree **into, int val)
{
    if (*into == NULL) {
        tree *ntr = MGC_NEW(into, tree);
        ntr->val = val;

    } else {
        tree *rel = *into;
        if (val > rel->val) {
            tree_insert(&(rel->right), val);

        } else if (val < rel->val) {
            tree_insert(&(rel->left), val);

        }

    }
}

void tree_remove(tree **into, int val)
{
    if (*into != NULL) {
        /* if this is val, remove it */
        if ((*into)->val == val) {
            MGC_FREE(*into);
            *into = NULL;
        }
    }
}

void tree_print(tree *t, int s)
{
    if (t == NULL) return;

    int i;
    for (i = 0; i < s; i++) {
        printf("| ");
    }
    printf("+%d\n", t->val);
    tree_print(t->left, s+1);
    tree_print(t->right, s+1);
}


int main()
{
    int i;
    tree **root = (tree **) pgcNewRoot(sizeof(void*), 1);

    for (i = 0; i < 100000000; i++) {
        /* add something ... */
        int x = rand() % 100;
        tree_insert(root, x);

        /* remove something */
        if (i % 10 == 0) {
            x = (rand() % 10) * 10;
            tree_remove(root, x);
        }

        /* tree_print(*root, 0); */
    }

    return 0;
}
