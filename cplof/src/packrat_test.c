#include <stdio.h>

#include "buffer.h"
#include "packrat.h"

void printParseResult(struct ParseResult *pr, char *input, int spcs)
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
    char **mul_a, **add_a, **term_a;

    mul_a = GC_MALLOC(4 * sizeof(char *));
    mul_a[0] = "add"; mul_a[1] = "/\\*/"; mul_a[2] = "add"; mul_a[3] = NULL;
    add_a = GC_MALLOC(4 * sizeof(char *));
    add_a[0] = "term"; add_a[1] = "/\\+/"; add_a[2] = "term"; add_a[3] = NULL;
    term_a = GC_MALLOC(2 * sizeof(char *));
    term_a[0] = "/[0-9]+/"; term_a[1] = NULL;

    mul = GC_MALLOC(2 * sizeof(char **));
    mul[0] = mul_a; mul[1] = NULL;
    add = GC_MALLOC(2 * sizeof(char **));
    add[0] = add_a; add[1] = NULL;
    term = GC_MALLOC(2 * sizeof(char **));
    term[0] = term_a; term[1] = NULL;

    newPackratNonterminal("mul", mul);
    newPackratNonterminal("add", add);
    newPackratNonterminal("term", term);
    newPackratRegexTerminal("/\\*/", "\\*");
    newPackratRegexTerminal("/\\+/", "\\+");
    newPackratRegexTerminal("/[0-9]+/", "[0-9]+");

    inp = "1+2*3+4";
    result = packratParse(getProduction("mul"), "INPUT", 0, 0, (unsigned char *) inp);
    printParseResult(result, inp, 0);

    return 0;
}
