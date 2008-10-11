#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gc.h"

/*
 * Layouts:
 *
 * GC space:
 *     1st cell = size of the space in words
 *     2nd cell = first free cell in words
 * GC'd object:
 *     1st cell = size or forwarding pointer
 *         if size, least significant bit is 1, 2nd two bits are generation, rest is the size in words
 *         otherwise, least significant bit is 0, all is forwarding pointer
 *     2nd cell is # of GC links (which must be at the head)
 *     3rd cell and on is the payload
 * Root (quite heavy as compared to the rest):
 *     1st cell = next pointer
 *     2nd cell = prev pointer
 *     3rd cell = size as in GC'd object, generation is all 1s
 *     4th cell = gclinks
 *     5th cell and on is the payload
 * Super-GC object (objects in the n+1th generation):
 *     Exactly as a GC'd object, but generation is all 1s
 */

/* global GC state */
void **gcSpaces[GC_GENERATIONS];

/* and its initializer */
void pgcInit()
{
    int i;
    for (i = 0; i < GC_GENERATIONS; i++) {
        gcSpaces[i] = NULL;
    }
}

/* list of roots */
void **roots = NULL;


void pgcCollect(int in);
void pgcTrace(void **at, int count, int in);
void *pgcCopy(void **into, void **gco, int to);


/* request a buffer in a space */
void *pgcNewIn(void **into, size_t sz, int gclinks, int in)
{
    /* make sure sz is on a void* boundary */
    if (sz % sizeof(void*) != 0) {
        sz += sizeof(void*);
        sz -= sz % sizeof(void*);
    }
    sz /= sizeof(void*);

    /* are we beyond PGC? */
    if (in >= GC_GENERATIONS) {
        /* easiest case, juse use libgc */
        void **ret = (void **) GC_malloc((sz+2) * sizeof(void*));
        if (ret == NULL) {
            perror("GC_malloc");
            exit(1);
        }

        /* still need to set the size, generation, etc */
        ret[0] = (void *) (
            (sz << (GC_GENERATION_BITS + 1)) |
            (0xFFFFFFFF >> (31-GC_GENERATION_BITS)));
        ret[1] = (void *) gclinks;

        return (void *) (ret + 2);
    }

    void **space;
    void **ret;

    if (gcSpaces[in] == NULL) {
        /* allocate the space */
        gcSpaces[in] = malloc(GC_DEF_SIZE);
        if (gcSpaces[in] == NULL) {
            perror("malloc");
            exit(1);
        }
        gcSpaces[in][0] = (void *) (GC_DEF_SIZE/sizeof(void*));
        gcSpaces[in][1] = (void *) 2;
    }
    space = gcSpaces[in];

    /* figure out if we have the space */
    while (space + (int) space[1] + sz + 2 > space + (int) space[0]) {
        /* no room, perform a collection */
        pgcCollect(in);
    }

    /* give this the next space */
    ret = space + (int) space[1];
    space[1] += (int) sz + 2;
    memset(ret, 0, (sz + 2) * sizeof(void*));

    /* put the size in the first slot, gclinks in second */
    ret[0] = (void *) ((sz<<(GC_GENERATION_BITS+1)) | (in<<1) | 1);
    ret[1] = (void *) gclinks;
    ret += 2;

    *into = (void *) ret;

    return (void *) ret;
}

/* request a buffer, by default in space 0 */
void *pgcNew(void **into, size_t sz, int gclinks)
{
    return pgcNewIn(into, sz, gclinks, 0);
}


/* get a new root (a malloc on a list) */
void *pgcNewRoot(size_t sz, int gclinks)
{
    void **ret = (void **) malloc(sz + (4*sizeof(void*)));
    if (ret == NULL) {
        perror("malloc");
        exit(1);
    }
    memset(ret, 0, sz + (4*sizeof(void*)));

    /* set the gclinks */
    ret[3] = (void *) gclinks;

    /* attach it to the current roots */
    /* first next */
    ret[0] = roots;
    if (roots)
        roots[1] = ret;
    ret[0] = (void *) roots;
    roots = ret;

    /* then prev */
    ret[1] = NULL;

    /* now the size */
    ret[2] = (void *) (
        ((sz/sizeof(void*)+1)<<(GC_GENERATION_BITS+1)) |
        (0xFFFFFFFF >> (31-GC_GENERATION_BITS)));

    return (void *) (ret + 4);
}

/* free a root */
void pgcFreeRoot(void *root)
{
    /* get the real root (4 words down) */
    void **rroot = ((void **) root) - 4;

    /* fix up next */
    if (rroot[0]) {
        ((void **) rroot[0])[1] = rroot[1];
    }

    /* fix up prev */
    if (rroot[1]) {
        ((void **) rroot[1])[0] = rroot[0];
    } else {
        roots = (void **) rroot[1];
    }

    /* then actually free it */
    free(rroot);
}

/* perform a collection in the level given */
void pgcCollect(int in)
{
    int i;

    /* go through the roots */
    void **root = roots;
    for (; root; root = (void **) root[0]) {

        /* trace these links */
        pgcTrace(root + 4, (int) root[4], in);
    }

    /* now properly blank out all these spaces */
    for (i = 0; i <= in; i++) {
        gcSpaces[i][1] = (void *) 2;
    }
}

/* trace a given set of links at the given level */
void pgcTrace(void **at, int count, int in)
{
    /* go through each link ... */
    int i;
    for (i = 0; i < count; i++) {
        void **link = (void **) at[i];

        /* if it's a null link, ignore it */
        if (link == NULL) continue;

        /* otherwise, we really care about the stuff two words down */
        link -= 2;

        /* get the real data */
        while (!((size_t) link[0] & 1)) {
            /* follow this forward */
            link = ((void **) link[0]) - 2;
        }

        /* now we may need to forward this */
        int gen = ((size_t) link[0] >> 1) & (0xFFFFFFFF >> (32-GC_GENERATION_BITS));
        if (gen <= in) {
            /* needs to be forwarded to [in]+1 */
            link[0] = pgcCopy(&(at[i]), link, in+1);

            /* then redirect ourself */
            link = ((void **) link[0]) - 2;

        }

    }
}

/* copy a GC'd object from one generation into another */
void *pgcCopy(void **into, void **gco, int to)
{
    /* get the size and gclinks from gco */
    size_t sz = ((size_t) gco[0] >> (GC_GENERATION_BITS+1));
    int gclinks = (int) gco[1];

    /* allocate */
    void *target = pgcNewIn(into, sz*sizeof(void*), gclinks, to);

    /* copy */
    memcpy(target, gco + 2, sz*sizeof(void*));

    return target;
}
