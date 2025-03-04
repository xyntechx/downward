#ifndef OPEN_LISTS_PRIMITIVE_GEN_OPEN_LIST_H
#define OPEN_LISTS_PRIMITIVE_GEN_OPEN_LIST_H

#include "../open_list_factory.h"

/*
  Open list indexed by a single int, using FIFO tie-breaking.

  Implemented as a map from int to deques.
*/

namespace primitive_gen_open_list {
class PrimitiveGenOpenListFactory : public OpenListFactory {
    std::shared_ptr<Evaluator> eval;
    bool pref_only;
public:
    PrimitiveGenOpenListFactory(
        const std::shared_ptr<Evaluator> &eval, bool pref_only);

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
