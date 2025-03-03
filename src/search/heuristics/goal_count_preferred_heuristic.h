#ifndef HEURISTICS_GOAL_COUNT_PREFERRED_HEURISTIC_H
#define HEURISTICS_GOAL_COUNT_PREFERRED_HEURISTIC_H

#include "../heuristic.h"

namespace goal_count_preferred_heuristic {
class GoalCountPreferredHeuristic : public Heuristic {
protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    GoalCountPreferredHeuristic(
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};
}

#endif
