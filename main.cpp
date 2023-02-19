#include "graphviz.h"

void create_tree(SUBGRAPH_CONTEXT, node_id parent, int x) {
    node_id current = NODE("%d", x);
    EDGE(parent, current);

    if (x == 0 || x == 1)
        return;

    create_tree(CURRENT_SUBGRAPH_CONTEXT, current, x - 1);
    create_tree(CURRENT_SUBGRAPH_CONTEXT, current, x - 2);
}

int main(void) {
    digraph my_graph = NEW_GRAPH({

        NEW_SUBGRAPH(RANK_NONE, {

            DEFAULT_NODE = {
                .style = STYLE_ROUNDED,
                .color = GRAPHVIZ_RED,
                .shape = SHAPE_BOX
            };

            DEFAULT_EDGE = {
                .color = GRAPHVIZ_ORANGE,
                .style = STYLE_SOLID,
            };

            node_id root = NODE("root");

            const int branch_factor = 3,
                      max_depth     = 2;

            create_tree(CURRENT_SUBGRAPH_CONTEXT, root, 15);

        });

    });

    printf("%s", digraph_render(&my_graph));
}
