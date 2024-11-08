#include "search_algorithm.h"

#include "evaluation_context.h"
#include "evaluator.h"

#include "algorithms/ordered_set.h"
#include "plugins/plugin.h"
#include "task_utils/successor_generator.h"
#include "task_utils/task_properties.h"
#include "tasks/root_task.h"
#include "utils/countdown_timer.h"
#include "utils/rng_options.h"
#include "utils/system.h"
#include "utils/timer.h"
#include <fstream>

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;
using utils::ExitCode;


static successor_generator::SuccessorGenerator &get_successor_generator(
    const TaskProxy &task_proxy, utils::LogProxy &log) {
    log << "Building successor generator..." << flush;
    int peak_memory_before = utils::get_peak_memory_in_kb();
    utils::Timer successor_generator_timer;
    successor_generator::SuccessorGenerator &successor_generator =
        successor_generator::g_successor_generators[task_proxy];
    successor_generator_timer.stop();
    log << "done!" << endl;
    int peak_memory_after = utils::get_peak_memory_in_kb();
    int memory_diff = peak_memory_after - peak_memory_before;
    log << "peak memory difference for successor generator creation: "
        << memory_diff << " KB" << endl
        << "time for successor generation creation: "
        << successor_generator_timer << endl;
    return successor_generator;
}

SearchAlgorithm::SearchAlgorithm(
    OperatorCost cost_type, int bound, double max_time,
    const string &description, utils::Verbosity verbosity)
    : description(description),
      status(IN_PROGRESS),
      solution_found(false),
      task(tasks::g_root_task),
      task_proxy(*task),
      log(utils::get_log_for_verbosity(verbosity)),
      state_registry(task_proxy),
      successor_generator(get_successor_generator(task_proxy, log)),
      search_space(state_registry, log),
      statistics(log),
      bound(bound),
      cost_type(cost_type),
      is_unit_cost(task_properties::is_unit_cost(task_proxy)),
      max_time(max_time),
      saved_macros({}) {
    if (bound < 0) {
        cerr << "error: negative cost bound " << bound << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    task_properties::print_variable_statistics(task_proxy);
}

SearchAlgorithm::SearchAlgorithm(const plugins::Options &opts) // TODO options object is needed for iterated search, the prototype for issue559 resolves this
    : description(opts.get_unparsed_config()),
      status(IN_PROGRESS),
      solution_found(false),
      task(tasks::g_root_task),
      task_proxy(*task),
      log(utils::get_log_for_verbosity(
              opts.get<utils::Verbosity>("verbosity"))),
      state_registry(task_proxy),
      successor_generator(get_successor_generator(task_proxy, log)),
      search_space(state_registry, log),
      statistics(log),
      cost_type(opts.get<OperatorCost>("cost_type")),
      is_unit_cost(task_properties::is_unit_cost(task_proxy)),
      max_time(opts.get<double>("max_time")) {
    if (opts.get<int>("bound") < 0) {
        cerr << "error: negative cost bound " << opts.get<int>("bound") << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    bound = opts.get<int>("bound");
    task_properties::print_variable_statistics(task_proxy);
}

SearchAlgorithm::~SearchAlgorithm() {
}

bool SearchAlgorithm::found_solution() const {
    return solution_found;
}

SearchStatus SearchAlgorithm::get_status() const {
    return status;
}

const Plan &SearchAlgorithm::get_plan() const {
    assert(solution_found);
    return plan;
}

void SearchAlgorithm::set_plan(const Plan &p) {
    solution_found = true;
    plan = p;
}

static ComposedMacro compose_macro(vector<OperatorProxy> sequence, int op_index) {
    vector<FactProxy> preconds;
    vector<Effect> effects;

    for (std::vector<OperatorProxy>::size_type op_idx = 0; op_idx < sequence.size(); ++op_idx) {
        OperatorProxy op = sequence[op_idx];
        vector<FactProxy> posts;
        for (Effect eff: effects) {
            posts.push_back(eff.fact);
        }

        vector<FactProxy> guaranteed_facts = posts;

        // Getting guaranteed_facts: all facts in posts and facts in preconds whose respective vars aren't in posts
        vector<int> post_vars;
        for (FactProxy pair: posts) {
            post_vars.push_back(pair.get_pair().var);
        }
        for (FactProxy precond: preconds) {
            if (std::find(post_vars.begin(), post_vars.end(), precond.get_pair().var) == post_vars.end()) {
                guaranteed_facts.push_back(precond);
            }
        }

        for (FactProxy precond: op.get_preconditions()) {
            // Include precond in overall preconds
            // if this fact has not been satisfied by the prev operator's post
            // and is not already in preconds
            bool is_fact_in_posts = std::find(posts.begin(), posts.end(), precond) != posts.end();
            bool is_fact_in_preconds = std::find(preconds.begin(), preconds.end(), precond) != preconds.end();
            if (!is_fact_in_posts && !is_fact_in_preconds) {
                preconds.push_back(precond);
            }
        }

        bool has_preconds = preconds.size() > 0;
        if (!has_preconds) {
            vector<Effect> updated_effects;
            for (EffectProxy op_eff : op.get_effects()) {
                vector<FactProxy> eff_conds;
                for (FactProxy conds : op_eff.get_conditions()) {
                    eff_conds.push_back(conds);
                }

                Effect eff = Effect(op_eff.get_fact(), eff_conds);

                updated_effects.push_back(eff);
            }


            vector<PrePost> postpres;
            if (op_idx > 0) {
                OperatorProxy prev_op = sequence[op_idx - 1];
                for (EffectProxy prev_eff: prev_op.get_effects()) {
                    vector<FactProxy> prev_eff_conds;
                    for (FactProxy cond : prev_eff.get_conditions()) {
                        prev_eff_conds.push_back(cond);
                    }
                    postpres.push_back({prev_eff.get_fact(), prev_eff_conds});
                }
            }

            for (Effect &eff: updated_effects) {
                for (PrePost pp: postpres) {
                    if (std::find(eff.conditions.begin(), eff.conditions.end(), pp.post) != eff.conditions.end()) {
                        eff.conditions = pp.pres;
                        break;
                    };
                }
            }

            vector<vector<FactProxy>> included_effconds;

            for (EffectProxy eff: op.get_effects()) {
                vector<FactProxy> eff_conds;
                for (FactProxy eff_cond : eff.get_conditions()) {
                    eff_conds.push_back(eff_cond);
                }
                included_effconds.push_back(eff_conds);
            }

            for (Effect old_eff: effects) {
                if (std::find(included_effconds.begin(), included_effconds.end(), old_eff.conditions) == included_effconds.end()) {
                    updated_effects.push_back(old_eff);
                    included_effconds.push_back(old_eff.conditions);
                }
            }

            effects = updated_effects;
        } else {
            for (EffectProxy eff: op.get_effects()) {
                vector<Effect> new_effects;
                for (Effect m_eff: effects) {
                    if (m_eff.fact.get_pair().var != eff.get_fact().get_pair().var) {
                        new_effects.push_back(m_eff);
                    }
                }

                vector<FactProxy> eff_conds;
                for (FactProxy conds : eff.get_conditions()) {
                    eff_conds.push_back(conds);
                }

                Effect exp_eff = Effect(eff.get_fact(), eff_conds);

                new_effects.push_back(exp_eff);
                effects = new_effects;
            }
        }
    }

    vector<FactProxy> posts;
    for (Effect eff: effects) {
        posts.push_back(eff.fact);
    }
    vector<int> post_vars;
    for (FactProxy fact: posts) {
        post_vars.push_back(fact.get_pair().var);
    }

    vector<FactProxy> prevails;
    for (FactProxy precond: preconds) {
        if (std::find(posts.begin(), posts.end(), precond) != posts.end()) {
            prevails.push_back(precond);
        } else if (std::find(post_vars.begin(), post_vars.end(), precond.get_pair().var) == post_vars.end()) {
            prevails.push_back(precond);
        }
    }

    vector<FactProxy> new_preconds;
    for (FactProxy pair: preconds) {
        if (std::find(prevails.begin(), prevails.end(), pair) == prevails.end()) {
            new_preconds.push_back(pair);
        }
    }

    vector<Effect> new_effects;
    for (Effect eff: effects) {
        if (std::find(prevails.begin(), prevails.end(), eff.fact) == prevails.end()) {
            new_effects.push_back(eff);
        }
    }

    ComposedMacro macro;
    macro.prevails = prevails;
    macro.preconditions = new_preconds; // tied to each effect (these conds must be true for operator to fire)
    macro.effects = new_effects; // post and eff condition(s) of each effect
    macro.cost = 1;
    macro.name = "macro" + to_string(op_index);
    macro.is_an_axiom = false;

    return macro;
}

void SearchAlgorithm::write_macros() {
    ofstream outfile("saved_macros.txt");

    OperatorsProxy ops = task_proxy.get_operators();
    int base_op_id = ops.size();

    for (Macro macro : saved_macros) {
        vector<OperatorProxy> seq;

        for (OperatorID op_id : macro.opid_sequence) {
            OperatorProxy op = task_proxy.get_operators()[op_id];
            seq.push_back(op);
        }

        ComposedMacro composed_macro = compose_macro(seq, base_op_id);

        outfile << "begin_macro" << '\n';
        outfile << "begin_macro_name" << '\n';
        outfile << composed_macro.name << '\n';

        outfile << "begin_macro_prevails" << '\n';
        for (FactProxy prevail : composed_macro.prevails) {
            outfile << prevail.get_pair() << '\n';
        }

        outfile << "begin_macro_effects" << '\n';
        for (Effect eff : composed_macro.effects) {
            outfile << "begin_eff_precondition" << '\n';
            for (FactProxy cond : composed_macro.preconditions) {
                if (cond.get_pair().var == eff.fact.get_pair().var) {
                    outfile << cond.get_pair() << '\n';
                    break;
                }
            }

            outfile << "begin_eff_conditions" << '\n';
            for (FactProxy cond : eff.conditions) {
                outfile << cond.get_pair() << '\n';
            }

            outfile << "begin_eff_fact" << '\n';
            outfile << eff.fact.get_pair() << '\n';
        }


        outfile << "begin_macro_cost" << '\n';
        outfile << composed_macro.cost << '\n';
        outfile << "begin_macro_isaxiom" << '\n';
        outfile << composed_macro.is_an_axiom << '\n';

        ++base_op_id;
    }
}

void SearchAlgorithm::search() {
    initialize();
    utils::CountdownTimer timer(max_time);
    int MACRO_SEARCH_BUDGET = 5000;
    int counter = 0;

    while (status == IN_PROGRESS) {
        status = step();
        if (timer.is_expired()) {
            log << "Time limit reached. Abort search." << endl;
            status = TIMEOUT;
            break;
        }

        ++counter;
        if (counter == MACRO_SEARCH_BUDGET) {
            write_macros();
            break;
        }
    }
    // TODO: Revise when and which search times are logged.
    log << "Actual search time: " << timer.get_elapsed_time() << endl;

    write_macros();
}

void SearchAlgorithm::save_macro_so_far(const State &state) {
    vector<OperatorID> path;
    vector<string> sequence;
    search_space.trace_path(state, path);

    for (OperatorID op_id : path) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        sequence.push_back(op.get_name());
    }

    int net_eff_size = 0;

    for (FactProxy init_state : task_proxy.get_initial_state()) {
        const VariableProxy var = init_state.get_variable();
        if (state[var] != init_state) {
            ++net_eff_size;
        }
    }

    if (net_eff_size == 0) {
        return;
    }

    Macro macro;
    macro.eff_size = net_eff_size;
    macro.sequence = sequence;
    macro.opid_sequence = path;

    saved_macros.push_back(macro);

    int MAX_SAVED_MACROS_SIZE = 400;

    if ((int) saved_macros.size() > MAX_SAVED_MACROS_SIZE) {
        struct eff_size_comp{
            bool operator()(const Macro& a, const Macro& b) const {
                return a.eff_size + a.sequence.size() < b.eff_size + b.sequence.size(); // max heap to easily remove worst macro
            }
        };
        std::make_heap(saved_macros.begin(), saved_macros.end(), eff_size_comp()); // make heap
        std::pop_heap(saved_macros.begin(), saved_macros.end(), eff_size_comp()); // move largest eff size macro to the end
        saved_macros.pop_back(); // actually remove largest eff size macro
    }
}

bool SearchAlgorithm::check_goal_and_set_plan(const State &state) {
    if (task_properties::is_goal_state(task_proxy, state)) {
        log << "Solution found!" << endl;
        Plan plan;
        search_space.trace_path(state, plan);
        set_plan(plan);
        return true;
    }
    return false;
}

void SearchAlgorithm::save_plan_if_necessary() {
    if (found_solution()) {
        plan_manager.save_plan(get_plan(), task_proxy);
    }
}

int SearchAlgorithm::get_adjusted_cost(const OperatorProxy &op) const {
    return get_adjusted_action_cost(op, cost_type, is_unit_cost);
}



void print_initial_evaluator_values(
    const EvaluationContext &eval_context) {
    eval_context.get_cache().for_each_evaluator_result(
        [] (const Evaluator *eval, const EvaluationResult &result) {
            if (eval->is_used_for_reporting_minima()) {
                eval->report_value_for_initial_state(result);
            }
        }
        );
}

/* TODO: merge this into add_options_to_feature when all search
         algorithms support pruning.

   Method doesn't belong here because it's only useful for certain derived classes.
   TODO: Figure out where it belongs and move it there. */
void add_search_pruning_options_to_feature(plugins::Feature &feature) {
    feature.add_option<shared_ptr<PruningMethod>>(
        "pruning",
        "Pruning methods can prune or reorder the set of applicable operators in "
        "each state and thereby influence the number and order of successor states "
        "that are considered.",
        "null()");
}

tuple<shared_ptr<PruningMethod>>
get_search_pruning_arguments_from_options(
    const plugins::Options &opts) {
    return make_tuple(opts.get<shared_ptr<PruningMethod>>("pruning"));
}

void add_search_algorithm_options_to_feature(
    plugins::Feature &feature, const string &description) {
    ::add_cost_type_options_to_feature(feature);
    feature.add_option<int>(
        "bound",
        "exclusive depth bound on g-values. Cutoffs are always performed according to "
        "the real cost, regardless of the cost_type parameter", "infinity");
    feature.add_option<double>(
        "max_time",
        "maximum time in seconds the search is allowed to run for. The "
        "timeout is only checked after each complete search step "
        "(usually a node expansion), so the actual runtime can be arbitrarily "
        "longer. Therefore, this parameter should not be used for time-limiting "
        "experiments. Timed-out searches are treated as failed searches, "
        "just like incomplete search algorithms that exhaust their search space.",
        "infinity");
    feature.add_option<string>(
        "description",
        "description used to identify search algorithm in logs",
        "\"" + description + "\"");
    utils::add_log_options_to_feature(feature);
}

tuple<OperatorCost, int, double, string, utils::Verbosity>
get_search_algorithm_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        ::get_cost_type_arguments_from_options(opts),
        make_tuple(
            opts.get<int>("bound"),
            opts.get<double>("max_time"),
            opts.get<string>("description")
            ),
        utils::get_log_arguments_from_options(opts)
        );
}

/* Method doesn't belong here because it's only useful for certain derived classes.
   TODO: Figure out where it belongs and move it there. */
void add_successors_order_options_to_feature(
    plugins::Feature &feature) {
    feature.add_option<bool>(
        "randomize_successors",
        "randomize the order in which successors are generated",
        "false");
    feature.add_option<bool>(
        "preferred_successors_first",
        "consider preferred operators first",
        "false");
    feature.document_note(
        "Successor ordering",
        "When using randomize_successors=true and "
        "preferred_successors_first=true, randomization happens before "
        "preferred operators are moved to the front.");
    utils::add_rng_options_to_feature(feature);
}

tuple<bool, bool, int> get_successors_order_arguments_from_options(
    const plugins::Options &opts) {
    return tuple_cat(
        make_tuple(
            opts.get<bool>("randomize_successors"),
            opts.get<bool>("preferred_successors_first")
            ),
        utils::get_rng_arguments_from_options(opts)
        );
}

static class SearchAlgorithmCategoryPlugin : public plugins::TypedCategoryPlugin<SearchAlgorithm> {
public:
    SearchAlgorithmCategoryPlugin() : TypedCategoryPlugin("SearchAlgorithm") {
        // TODO: Replace add synopsis for the wiki page.
        // document_synopsis("...");
    }
}
_category_plugin;

void collect_preferred_operators(
    EvaluationContext &eval_context,
    Evaluator *preferred_operator_evaluator,
    ordered_set::OrderedSet<OperatorID> &preferred_operators) {
    if (!eval_context.is_evaluator_value_infinite(preferred_operator_evaluator)) {
        for (OperatorID op_id : eval_context.get_preferred_operators(preferred_operator_evaluator)) {
            preferred_operators.insert(op_id);
        }
    }
}
