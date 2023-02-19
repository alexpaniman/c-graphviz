#include "graphviz.h"

template <typename E>
digraph create_linked_list_graph(linked_list<E>* list) {
    return NEW_GRAPH({
        NEW_SUBGRAPH(RANK_SAME, {
            DEFAULT_NODE = {
                .style = STYLE_ROUNDED,
                .color = GRAPHVIZ_BLUE,
                .shape = SHAPE_BOX
            };

            node_id nodes[list->capacity + 1];

            for (int i = 1; i <= (int) list->capacity + 1; ++ i) {
                const element<E>* el = &list->elements[i];
                nodes[i] = NODE(R"(node_%03d [label = <<table border="0" cellborder="1" cellspacing="0">
                            <tr> <td port="index" colspan="2"> %d </td> </tr>
                            <tr> <td> elem </td> <td port="elem"> %d </td> </tr>
                            <tr> <td> prev </td> <td port="prev"> %d </td> </tr>
                            <tr> <td> next </td> <td port="next"> %d </td> </tr>
                        </table>>];)" "\n", i, i, el->element, el->prev_index, el->next_index);
            }

            for (int i = list->elements[0].next_index != 0? 0 : 1; i <= (int) list->capacity + 1; ++ i) {
                const element<E>* el = &list->elements[i];

                // if (el->next_index == 0)
                //     EDGE(nodes[el->]);
                //     fprintf(file, "\t\t node_%03d:next -> node_%03d; \n", i, el->next_index);

                // if (el->prev_index == 0)
                //     fprintf(file, "\t\t node_%03d:prev -> node_%03d; \n", i, el->prev_index);
            }
        });
    });
}
