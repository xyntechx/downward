#include "goal_count_preferred_heuristic.h"

#include "../plugins/plugin.h"

#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace goal_count_preferred_heuristic {
GoalCountPreferredHeuristic::GoalCountPreferredHeuristic(
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity) {
    if (log.is_at_least_normal()) {
        log << "Initializing goal count preferred heuristic..." << endl;
    }
}

int GoalCountPreferredHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int unsatisfied_goal_count = 0;

    for (FactProxy goal : task_proxy.get_goals()) {
        const VariableProxy var = goal.get_variable();
        if (state[var] != goal) {
            ++unsatisfied_goal_count;
        }
    }
    return unsatisfied_goal_count;
}

class GoalCountPreferredHeuristicFeature
    : public plugins::TypedFeature<Evaluator, GoalCountPreferredHeuristic> {
public:
    GoalCountPreferredHeuristicFeature() : TypedFeature("goalcountpreferred") {
        document_title("Goal count preferred heuristic");

        add_heuristic_options_to_feature(*this, "goalcountpreferred");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "yes");
    }

    virtual shared_ptr<GoalCountPreferredHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<GoalCountPreferredHeuristic>(
            get_heuristic_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<GoalCountPreferredHeuristicFeature> _plugin;
}
