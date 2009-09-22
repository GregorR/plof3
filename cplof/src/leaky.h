#ifndef LEAKY_H
#define LEAKY_H

/* types to help with memory-leak reduction */

/* this is to be formed into an abstract stack to detect what may be leaked */
struct Leaky {
    struct Leaky *dup;
    unsigned char leaks;
};

#endif
