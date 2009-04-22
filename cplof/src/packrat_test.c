#include <stdio.h>

#include "buffer.h"
#include "packrat.h"

void printParseResult(struct ParseResult *pr, unsigned char *input, int spcs)
{
    int i;

    for (i = 0; i < spcs; i++) printf(" ");

    printf("%s: %.*s\n", pr->production->name,
                         (int) (pr->consumedTo - pr->consumedFrom),
                         input + pr->consumedFrom);

    if (pr->subResults)
        for (i = 0; pr->subResults[i]; i++)
            printParseResult(pr->subResults[i], input, spcs+1);
}

int main(int argc, char **argv)
{
    struct ParseResult *result;

    char ***mul, ***add, ***term, *inp;
    char **mul_a, **mul_b, **add_a, **add_b, **term_a;

    mul_a = GC_MALLOC(4 * sizeof(unsigned char *));
    mul_a[0] = "mul"; mul_a[1] = "/\\*/"; mul_a[2] = "add"; mul_a[3] = NULL;
    mul_b = GC_MALLOC(2 * sizeof(unsigned char *));
    mul_b[0] = "add"; mul_b[1] = NULL;
    add_a = GC_MALLOC(4 * sizeof(unsigned char *));
    add_a[0] = "add"; add_a[1] = "/\\+/"; add_a[2] = "term"; add_a[3] = NULL;
    add_b = GC_MALLOC(2 * sizeof(unsigned char *));
    add_b[0] = "term"; add_b[1] = NULL;
    term_a = GC_MALLOC(2 * sizeof(unsigned char *));
    term_a[0] = "/[0-9]+/"; term_a[1] = NULL;

    mul = GC_MALLOC(3 * sizeof(unsigned char **));
    mul[0] = mul_a; mul[1] = mul_b; mul[2] = NULL;
    add = GC_MALLOC(3 * sizeof(unsigned char **));
    add[0] = add_a; add[1] = add_b; add[2] = NULL;
    term = GC_MALLOC(2 * sizeof(unsigned char **));
    term[0] = term_a; term[1] = NULL;

    newPackratNonterminal((unsigned char *) "mul", (unsigned char ***) mul);
    newPackratNonterminal((unsigned char *) "add", (unsigned char ***) add);
    newPackratNonterminal((unsigned char *) "term", (unsigned char ***) term);
    newPackratRegexTerminal((unsigned char *) "/\\*/", (unsigned char *) "\\*");
    newPackratRegexTerminal((unsigned char *) "/\\+/", (unsigned char *) "\\+");
    newPackratRegexTerminal((unsigned char *) "/[0-9]+/", (unsigned char *) "[0-9]+");

    inp = "1+2*3+4";
    result = packratParse(getProduction((unsigned char *) "mul"), (unsigned char *) "INPUT", 0, 0, (unsigned char *) inp);
    printParseResult(result, (unsigned char *) inp, 0);

    return 0;
}
