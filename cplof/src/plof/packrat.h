/*
 * A Plof-suitable packrat parser (memoized recursive descent parser)
 *
 * Copyright (C) 2009, 2010 Gregor Richards
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef PACKRAT_H
#define PACKRAT_H

struct Production;
struct ParseContext;

/* The type for the underlying parser functions, returns a NULL-terminated
 * array of (potential) parse results */
typedef struct ParseResult **(*Parser) (struct ParseContext *ctx,
                                        struct Production *production,
                                        unsigned char *file, int line, int col,
                                        unsigned char *input, size_t off);

/* A parsing context. Used mainly for detecting parse errors, this gets filled
 * in with the latest valid parse */
struct ParseContext {
    /* starting location AFTER the latest successful parse */
    size_t loc;
    int line, col;

    /* the production currently being parsed */
    struct Production *current;

    /* the production which was /expected/ */
    struct Production *expected;
};

/* A production. Could be a terminal or a nonterminal, includes a
 * reference to the relevant parsing function and arg. Grammar elements form a
 * tree for easy indexing */
struct Production {
    /* the name of this production */
    unsigned char *name;

    /* the tree */
    struct Production *left, *right;

    /* the memoization cache */
    size_t cachesz;
    struct ParseResult ***cache;

    /* the underlying parser function */
    Parser parser;

    /* the argument, and user argument */
    void *arg, *userarg;

    /* any subproductions, for clearing */
    struct Production **sub;
};

extern struct Production *productions;

/* A parsing result, including the particular production, file, etc */
struct ParseResult {
    struct ParseContext *ctx;
    struct Production *production;
    struct ParseResult **subResults;

    /* what was parsed */
    unsigned char *file;
    int sline, eline;
    int scol, ecol;

    /* the option chosen, for nondeterministic nonterminals */
    int choice;

    /* range consumed by this parse, up to but not including consumedTo */
    size_t consumedFrom, consumedTo;
};

/* get a production matching a particular name. Always returns something, but
 * may have NULL functionality */
struct Production *getProduction(const unsigned char *name);

/* remove all productions with the given name */
void delProduction(const unsigned char *name);

/* remove ALL productions */
void delAllProductions();

/* parse using the specified production */
struct ParseResult *packratParse(struct ParseContext *ctx,
                                 struct Production *production,
                                 unsigned char *file,
                                 int line, int col,
                                 unsigned char *input);

/* built-in parsers */
struct ParseResult **packratNonterminal(struct ParseContext *ctx,
                                        struct Production *production,
                                        unsigned char *file, int line, int col,
                                        unsigned char *input, size_t off);
struct ParseResult **packratRegexTerminal(struct ParseContext *ctx,
                                          struct Production *production,
                                          unsigned char *file, int line, int col,
                                          unsigned char *input, size_t off);

/* and generaters for them */
struct Production *newPackratNonterminal(unsigned char *name, unsigned char ***sub);
struct Production *newPackratRegexTerminal(unsigned char *name, unsigned char *regex);

#endif
