#include <stdio.h>
#include "conditions.h"

static void print_root_children(const rule_program *prog)
{
    const rule_node *root;
    int index;

    root = &prog->nodes[prog->root];
    printf("root_child_priorities:");
    index = 0;
    while (index < root->child_count)
    {
        printf(" %d", prog->nodes[root->children[index]].priority);
        ++index;
    }
    printf("\n");
}

static int run_example(void)
{
    game_desc game;
    rule_program prog;
    char err[CONDITIONS_ERR_SIZE];
    int result;

    result = parse_conditions_file("tests/fixtures/example.json", &game, &prog, err, sizeof(err));
    if (result != 0)
    {
        printf("example.json failed: %s\n", err);
        return 1;
    }
    printf("node_count=%d\n", prog.node_count);
    printf("root_kind=%d\n", prog.nodes[prog.root].kind);
    printf("needs_planets=%d\n", prog.needs_planets);
    printf("needs_themes=%d\n", prog.needs_themes);
    print_root_children(&prog);
    return 0;
}

static void run_invalid(void)
{
    game_desc game;
    rule_program prog;
    char err[CONDITIONS_ERR_SIZE];
    int result;

    result = parse_conditions_file("tests/fixtures/invalid_enum.json", &game, &prog, err, sizeof(err));
    printf("invalid_enum result=%d err=\"%s\"\n", result, err);
}

int main(void)
{
    if (run_example() != 0)
    {
        return 1;
    }
    run_invalid();
    return 0;
}
