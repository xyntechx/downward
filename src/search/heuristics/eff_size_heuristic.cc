#include "eff_size_heuristic.h"

#include "../plugins/plugin.h"

#include "../utils/logging.h"

#include <iostream>
using namespace std;

namespace eff_size_heuristic {
EffSizeHeuristic::EffSizeHeuristic(
    const shared_ptr<AbstractTask> &transform, bool cache_estimates,
    const string &description, utils::Verbosity verbosity)
    : Heuristic(transform, cache_estimates, description, verbosity) {
    if (log.is_at_least_normal()) {
        log << "Initializing effect size heuristic..." << endl;
    }
}

int EffSizeHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int net_eff_size = 0;

    for (FactProxy init_state : task_proxy.get_initial_state()) {
        const VariableProxy var = init_state.get_variable();
        if (state[var] != init_state) {
            ++net_eff_size;
        }
    }

    // Same as initial state
    if (net_eff_size == 0) {
        return 999;
    }

    return net_eff_size;
}

class EffSizeHeuristicFeature
    : public plugins::TypedFeature<Evaluator, EffSizeHeuristic> {
public:
    EffSizeHeuristicFeature() : TypedFeature("effsize") {
        document_title("Effect size heuristic");

        add_heuristic_options_to_feature(*this, "effsize");

        document_language_support("action costs", "ignored by design");
        document_language_support("conditional effects", "supported");
        document_language_support("axioms", "supported");

        document_property("admissible", "no");
        document_property("consistent", "no");
        document_property("safe", "yes");
        document_property("preferred operators", "no");
    }

    virtual shared_ptr<EffSizeHeuristic> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<EffSizeHeuristic>(
            get_heuristic_arguments_from_options(opts)
            );
    }
};

static plugins::FeaturePlugin<EffSizeHeuristicFeature> _plugin;
}
