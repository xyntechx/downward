#ifndef HEURISTICS_EFF_SIZE_HEURISTIC_H
#define HEURISTICS_EFF_SIZE_HEURISTIC_H

#include "../heuristic.h"

namespace eff_size_heuristic {
class EffSizeHeuristic : public Heuristic {
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    EffSizeHeuristic(
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};
}

#endif
