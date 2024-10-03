#include "root_task.h"

#include "../state_registry.h"

#include "../plugins/plugin.h"
#include "../utils/collections.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <set>
#include <unordered_set>
#include <vector>


using namespace std;
using utils::ExitCode;

namespace tasks {
static const int PRE_FILE_VERSION = 3;
shared_ptr<AbstractTask> g_root_task = nullptr;

struct ExplicitVariable {
    int domain_size;
    string name;
    vector<string> fact_names;
    int axiom_layer;
    int axiom_default_value;

    explicit ExplicitVariable(istream &in);
};


struct ExplicitEffect {
    FactPair fact;
    vector<FactPair> conditions;

    ExplicitEffect(int var, int value, vector<FactPair> &&conditions);
};


struct ExplicitOperator {
    vector<FactPair> preconditions;
    vector<ExplicitEffect> effects;
    int cost;
    string name;
    bool is_an_axiom;

    void read_pre_post(istream &in);
    ExplicitOperator(istream &in, bool is_an_axiom, bool use_metric);
};


struct PrePost {
    FactPair pre;
    FactPair post;
};


class RootTask : public AbstractTask {
    vector<ExplicitVariable> variables;
    // TODO: think about using hash sets here.
    vector<vector<set<FactPair>>> mutexes;
    vector<ExplicitOperator> operators;
    vector<ExplicitOperator> axioms;
    vector<int> initial_state_values;
    vector<FactPair> goals;

    const ExplicitVariable &get_variable(int var) const;
    const ExplicitEffect &get_effect(int op_id, int effect_id, bool is_axiom) const;
    const ExplicitOperator &get_operator_or_axiom(int index, bool is_axiom) const;

public:
    explicit RootTask(istream &in);

    virtual int get_num_variables() const override;
    virtual string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;
    virtual string get_fact_name(const FactPair &fact) const override;
    virtual bool are_facts_mutex(
        const FactPair &fact1, const FactPair &fact2) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual string get_operator_name(
        int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(
        int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(
        int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual int convert_operator_index(
        int index, const AbstractTask *ancestor_task) const override;

    virtual int get_num_axioms() const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual vector<int> get_initial_state_values() const override;
    virtual void convert_ancestor_state_values(
        vector<int> &values,
        const AbstractTask *ancestor_task) const override;
};


static void check_fact(const FactPair &fact, const vector<ExplicitVariable> &variables) {
    if (!utils::in_bounds(fact.var, variables)) {
        cerr << "Invalid variable id: " << fact.var << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    if (fact.value < 0 || fact.value >= variables[fact.var].domain_size) {
        cerr << "Invalid value for variable " << fact.var << ": " << fact.value << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

static void check_facts(const vector<FactPair> &facts, const vector<ExplicitVariable> &variables) {
    for (FactPair fact : facts) {
        check_fact(fact, variables);
    }
}

static void check_facts(const ExplicitOperator &action, const vector<ExplicitVariable> &variables) {
    check_facts(action.preconditions, variables);
    for (const ExplicitEffect &eff : action.effects) {
        check_fact(eff.fact, variables);
        check_facts(eff.conditions, variables);
    }
}

static void check_magic(istream &in, const string &magic) {
    string word;
    in >> word;
    if (word != magic) {
        cerr << "Failed to match magic word '" << magic << "'." << endl
             << "Got '" << word << "'." << endl;
        if (magic == "begin_version") {
            cerr << "Possible cause: you are running the planner "
                 << "on a translator output file from " << endl
                 << "an older version." << endl;
        }
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

static vector<FactPair> read_facts(istream &in) {
    int count;
    in >> count;
    vector<FactPair> conditions;
    conditions.reserve(count);
    for (int i = 0; i < count; ++i) {
        FactPair condition = FactPair::no_fact;
        in >> condition.var >> condition.value;
        conditions.push_back(condition);
    }
    return conditions;
}

ExplicitVariable::ExplicitVariable(istream &in) {
    check_magic(in, "begin_variable");
    in >> name;
    in >> axiom_layer;
    in >> domain_size;
    in >> ws;
    fact_names.resize(domain_size);
    for (int i = 0; i < domain_size; ++i)
        getline(in, fact_names[i]);
    check_magic(in, "end_variable");
}


ExplicitEffect::ExplicitEffect(
    int var, int value, vector<FactPair> &&conditions)
    : fact(var, value), conditions(move(conditions)) {
}


void ExplicitOperator::read_pre_post(istream &in) {
    vector<FactPair> conditions = read_facts(in);
    int var, value_pre, value_post;
    in >> var >> value_pre >> value_post;
    if (value_pre != -1) {
        preconditions.emplace_back(var, value_pre);
    }
    effects.emplace_back(var, value_post, move(conditions));
}

ExplicitOperator::ExplicitOperator(istream &in, bool is_an_axiom, bool use_metric)
    : is_an_axiom(is_an_axiom) {
    if (!is_an_axiom) {
        check_magic(in, "begin_operator");
        in >> ws;
        getline(in, name);
        preconditions = read_facts(in);
        int count;
        in >> count;
        effects.reserve(count);
        for (int i = 0; i < count; ++i) {
            read_pre_post(in);
        }

        int op_cost;
        in >> op_cost;
        cost = use_metric ? op_cost : 1;
        check_magic(in, "end_operator");
    } else {
        name = "<axiom>";
        cost = 0;
        check_magic(in, "begin_rule");
        read_pre_post(in);
        check_magic(in, "end_rule");
    }
    assert(cost >= 0);
}

static void read_and_verify_version(istream &in) {
    int version;
    check_magic(in, "begin_version");
    in >> version;
    check_magic(in, "end_version");
    if (version != PRE_FILE_VERSION) {
        cerr << "Expected translator output file version " << PRE_FILE_VERSION
             << ", got " << version << "." << endl
             << "Exiting." << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
}

static bool read_metric(istream &in) {
    bool use_metric;
    check_magic(in, "begin_metric");
    in >> use_metric;
    check_magic(in, "end_metric");
    return use_metric;
}

static vector<ExplicitVariable> read_variables(istream &in) {
    int count;
    in >> count;
    vector<ExplicitVariable> variables;
    variables.reserve(count);
    for (int i = 0; i < count; ++i) {
        variables.emplace_back(in);
    }
    return variables;
}

static vector<vector<set<FactPair>>> read_mutexes(istream &in, const vector<ExplicitVariable> &variables) {
    vector<vector<set<FactPair>>> inconsistent_facts(variables.size());
    for (size_t i = 0; i < variables.size(); ++i)
        inconsistent_facts[i].resize(variables[i].domain_size);

    int num_mutex_groups;
    in >> num_mutex_groups;

    /*
      NOTE: Mutex groups can overlap, in which case the same mutex
      should not be represented multiple times. The current
      representation takes care of that automatically by using sets.
      If we ever change this representation, this is something to be
      aware of.
    */
    for (int i = 0; i < num_mutex_groups; ++i) {
        check_magic(in, "begin_mutex_group");
        int num_facts;
        in >> num_facts;
        vector<FactPair> invariant_group;
        invariant_group.reserve(num_facts);
        for (int j = 0; j < num_facts; ++j) {
            int var;
            int value;
            in >> var >> value;
            invariant_group.emplace_back(var, value);
        }
        check_magic(in, "end_mutex_group");
        for (const FactPair &fact1 : invariant_group) {
            for (const FactPair &fact2 : invariant_group) {
                if (fact1.var != fact2.var) {
                    /* The "different variable" test makes sure we
                       don't mark a fact as mutex with itself
                       (important for correctness) and don't include
                       redundant mutexes (important to conserve
                       memory). Note that the translator (at least
                       with default settings) removes mutex groups
                       that contain *only* redundant mutexes, but it
                       can of course generate mutex groups which lead
                       to *some* redundant mutexes, where some but not
                       all facts talk about the same variable. */
                    inconsistent_facts[fact1.var][fact1.value].insert(fact2);
                }
            }
        }
    }
    return inconsistent_facts;
}

static vector<FactPair> read_goal(istream &in) {
    check_magic(in, "begin_goal");
    vector<FactPair> goals = read_facts(in);
    check_magic(in, "end_goal");
    if (goals.empty()) {
        cerr << "Task has no goal condition!" << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    return goals;
}

static vector<ExplicitOperator> read_actions(
    istream &in, bool is_axiom, bool use_metric,
    const vector<ExplicitVariable> &variables) {
    int count;
    in >> count;
    vector<ExplicitOperator> actions;
    actions.reserve(count);
    for (int i = 0; i < count; ++i) {
        actions.emplace_back(in, is_axiom, use_metric);
        check_facts(actions.back(), variables);
    }
    return actions;
}

// static vector<ExplicitOperator> create_double_macros(vector<ExplicitOperator> operators) {
//     vector<ExplicitOperator> new_ops;
//     new_ops.reserve(operators.size() * 2);

//     for (tasks::ExplicitOperator op: operators) {
//         new_ops.push_back(op);
//         ExplicitOperator double_op = op;
//         double_op.name = "double_" + op.name;

//         vector<PrePost> preposts;

//         for (ExplicitEffect eff: op.effects) {
//             FactPair post = eff.fact;
//             for (FactPair pre: eff.conditions) {
//                 PrePost pp = {pre, post};
//                 preposts.push_back(pp);
//             }
//         }

//         for (ExplicitEffect &eff: double_op.effects) {
//             FactPair intermediate_post = eff.fact;
//             for (PrePost pp: preposts) {
//                 if (pp.pre == intermediate_post) {
//                     eff.fact = pp.post;
//                     break;
//                 };
//             }
//         }

//         new_ops.push_back(double_op);
//     }

//     vector<ExplicitOperator> final_ops;
//     final_ops.reserve(new_ops.size());

//     for (u_long i = 0; i < new_ops.size(); ++i) {
//         ExplicitOperator op1 = new_ops[i];
//         bool is_duplicate = false;

//         vector<FactPair> pres1;
//         vector<FactPair> posts1;
//         for (ExplicitEffect eff: op1.effects) {
//             for (FactPair pre: eff.conditions) {
//                 pres1.push_back(pre);
//             }
//             posts1.push_back(eff.fact);
//         }

//         for (u_long j = i+1; j < new_ops.size(); ++j) {
//             ExplicitOperator op2 = new_ops[j];

//             vector<FactPair> pres2;
//             vector<FactPair> posts2;
//             for (ExplicitEffect eff: op2.effects) {
//                 for (FactPair pre: eff.conditions) {
//                     pres2.push_back(pre);
//                 }
//                 posts2.push_back(eff.fact);
//             }

//             if (pres1 == pres2 && posts1 == posts2) {
//                 // If elements and their order match
//                 is_duplicate = true;
//                 break;
//             }
//         }

//         if (!is_duplicate) {
//             final_ops.push_back(op1);
//         }
//     }

//     return final_ops;
// }

static vector<vector<ExplicitOperator>> generate_double_macros(vector<ExplicitOperator> operators) {
    // Macros generated can be valid or invalid (checked by compose_macro)
    // Only used temporarily; in the future, we will learn macros using BFS
    vector<vector<ExplicitOperator>> macros;

    for (ExplicitOperator op1: operators) {
        for (ExplicitOperator op2: operators) {
            macros.push_back({op1, op2});
        }
    }

    return macros;
}

static ExplicitOperator compose_macro(vector<ExplicitOperator> sequence) {
    ExplicitOperator macro = sequence[0]; // arbitrarily initialize with 0th operator
    macro.name = "";

    vector<FactPair> preconds;
    vector<ExplicitEffect> effects;

    for (ExplicitOperator op: sequence) {
        vector<FactPair> posts;
        for (ExplicitEffect eff: effects) {
            posts.push_back(eff.fact);
        }

        vector<FactPair> guaranteed_facts = posts;

        // Getting guaranteed_facts: all facts in posts and facts in preconds whose respective vars aren't in posts
        vector<int> post_vars;
        for (FactPair pair: posts) {
            post_vars.push_back(pair.var);
        }
        for (FactPair pair: preconds) {
            if (std::find(post_vars.begin(), post_vars.end(), pair.var) == post_vars.end()) {
                guaranteed_facts.push_back(pair);
            }
        }

        for (FactPair precond: op.preconditions) {
            // Check whether any precondition is violated by guaranteed_facts
            for (FactPair g_fact: guaranteed_facts) {
                if (precond.var == g_fact.var && precond.value != g_fact.value) {
                    // invalid macro
                    macro.name = "INVALID_MACRO";
                    return macro;
                }
            }

            // Include precond in overall preconds
            // if this fact has not been satisfied by the prev operator's post
            // and is not already in preconds
            bool is_fact_in_posts = std::find(posts.begin(), posts.end(), precond) != posts.end();
            bool is_fact_in_preconds = std::find(preconds.begin(), preconds.end(), precond) != preconds.end();
            if (!is_fact_in_posts && !is_fact_in_preconds) {
                preconds.push_back(precond);
            }
        }

        for (ExplicitEffect eff: op.effects) {
            // Check whether any effect condition is violated
            int count_violated = 0;
            for (FactPair g_fact: guaranteed_facts) {
                for (FactPair c_fact: eff.conditions) {
                    if (g_fact.var == c_fact.var && g_fact.value != c_fact.value) {
                        ++count_violated;
                        break;
                    }
                }
            }

            // Run post only if the effect's conditions are satisfied
            // Note that macro is still valid even if these conditions are NOT satisfied
            // because these conditions are just to determine whether this particular effect can run
            if (count_violated == 0) {
                vector<ExplicitEffect> new_effects;
                for (ExplicitEffect m_eff: effects) {
                    if (m_eff.fact.var != eff.fact.var) {
                        // Overwriting an old effect if it has the same var as the new effect
                        new_effects.push_back(m_eff);
                    }
                }
                new_effects.push_back(eff); // Overwriting old effect (continued)
                effects = new_effects;
            }
        }
        macro.name += op.name;
    }

    vector<FactPair> posts;
    for (ExplicitEffect eff: effects) {
        posts.push_back(eff.fact);
    }
    vector<int> post_vars;
    for (FactPair pair: posts) {
        post_vars.push_back(pair.var);
    }

    vector<FactPair> prevails;
    for (FactPair precond: preconds) {
        if (std::find(posts.begin(), posts.end(), precond) != posts.end()) {
            prevails.push_back(precond);
        } else if (std::find(post_vars.begin(), post_vars.end(), precond.var) == post_vars.end()) {
            prevails.push_back(precond);
        }
    }

    vector<FactPair> new_preconds;
    for (FactPair pair: preconds) {
        if (std::find(prevails.begin(), prevails.end(), pair) == prevails.end()) {
            new_preconds.push_back(pair);
        }
    }

    vector<ExplicitEffect> new_effects;
    for (ExplicitEffect eff: effects) {
        if (std::find(prevails.begin(), prevails.end(), eff.fact) == prevails.end()) {
            new_effects.push_back(eff);
        }
    }

    macro.effects = new_effects;
    macro.preconditions = {};
    // In downward, prevails come first before non-prevail preconds in the preconditions field of each operator
    for (FactPair prevail: prevails) {
        macro.preconditions.push_back(prevail);
    }
    for (FactPair precond: new_preconds) {
        macro.preconditions.push_back(precond);
    }

    return macro;
}

RootTask::RootTask(istream &in) {
    read_and_verify_version(in);
    bool use_metric = read_metric(in);
    variables = read_variables(in);
    int num_variables = variables.size();

    mutexes = read_mutexes(in, variables);

    initial_state_values.resize(num_variables);
    check_magic(in, "begin_state");
    for (int i = 0; i < num_variables; ++i) {
        in >> initial_state_values[i];
    }
    check_magic(in, "end_state");

    for (int i = 0; i < num_variables; ++i) {
        variables[i].axiom_default_value = initial_state_values[i];
    }

    goals = read_goal(in);
    check_facts(goals, variables);
    operators = read_actions(in, false, use_metric, variables);
    
    vector<vector<ExplicitOperator>> potential_macros = generate_double_macros(operators);
    for (vector<ExplicitOperator> seq: potential_macros) {
        ExplicitOperator macro = compose_macro(seq);
        if (macro.name != "INVALID_MACRO")
            operators.push_back(macro);
    }

    // std::cout << "Operators: ";
    for (tasks::ExplicitOperator op: operators) {
        std::cout << op.name << ' ' << op.preconditions << "         ";
        for (ExplicitEffect eff: op.effects) {
            // std::cout << eff.conditions << ' ' << eff.fact << ' ';
            std::cout << eff.fact << ' ';
        }
        std::cout << '\n';
    }
    std::cout << '\n';

    axioms = read_actions(in, true, use_metric, variables);
    /* TODO: We should be stricter here and verify that we
       have reached the end of "in". */

    /*
      HACK: We use a TaskProxy to access g_axiom_evaluators here which assumes
      that this task is completely constructed.
    */
    AxiomEvaluator &axiom_evaluator = g_axiom_evaluators[TaskProxy(*this)];
    axiom_evaluator.evaluate(initial_state_values);
}

const ExplicitVariable &RootTask::get_variable(int var) const {
    assert(utils::in_bounds(var, variables));
    return variables[var];
}

const ExplicitEffect &RootTask::get_effect(
    int op_id, int effect_id, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_id, is_axiom);
    assert(utils::in_bounds(effect_id, op.effects));
    return op.effects[effect_id];
}

const ExplicitOperator &RootTask::get_operator_or_axiom(
    int index, bool is_axiom) const {
    if (is_axiom) {
        assert(utils::in_bounds(index, axioms));
        return axioms[index];
    } else {
        assert(utils::in_bounds(index, operators));
        return operators[index];
    }
}

int RootTask::get_num_variables() const {
    return variables.size();
}

string RootTask::get_variable_name(int var) const {
    return get_variable(var).name;
}

int RootTask::get_variable_domain_size(int var) const {
    return get_variable(var).domain_size;
}

int RootTask::get_variable_axiom_layer(int var) const {
    return get_variable(var).axiom_layer;
}

int RootTask::get_variable_default_axiom_value(int var) const {
    return get_variable(var).axiom_default_value;
}

string RootTask::get_fact_name(const FactPair &fact) const {
    assert(utils::in_bounds(fact.value, get_variable(fact.var).fact_names));
    return get_variable(fact.var).fact_names[fact.value];
}

bool RootTask::are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const {
    if (fact1.var == fact2.var) {
        // Same variable: mutex iff different value.
        return fact1.value != fact2.value;
    }
    assert(utils::in_bounds(fact1.var, mutexes));
    assert(utils::in_bounds(fact1.value, mutexes[fact1.var]));
    return bool(mutexes[fact1.var][fact1.value].count(fact2));
}

int RootTask::get_operator_cost(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).cost;
}

string RootTask::get_operator_name(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).name;
}

int RootTask::get_num_operators() const {
    return operators.size();
}

int RootTask::get_num_operator_preconditions(int index, bool is_axiom) const {
    return get_operator_or_axiom(index, is_axiom).preconditions.size();
}

FactPair RootTask::get_operator_precondition(
    int op_index, int fact_index, bool is_axiom) const {
    const ExplicitOperator &op = get_operator_or_axiom(op_index, is_axiom);
    assert(utils::in_bounds(fact_index, op.preconditions));
    return op.preconditions[fact_index];
}

int RootTask::get_num_operator_effects(int op_index, bool is_axiom) const {
    return get_operator_or_axiom(op_index, is_axiom).effects.size();
}

int RootTask::get_num_operator_effect_conditions(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).conditions.size();
}

FactPair RootTask::get_operator_effect_condition(
    int op_index, int eff_index, int cond_index, bool is_axiom) const {
    const ExplicitEffect &effect = get_effect(op_index, eff_index, is_axiom);
    assert(utils::in_bounds(cond_index, effect.conditions));
    return effect.conditions[cond_index];
}

FactPair RootTask::get_operator_effect(
    int op_index, int eff_index, bool is_axiom) const {
    return get_effect(op_index, eff_index, is_axiom).fact;
}

int RootTask::convert_operator_index(
    int index, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid operator ID conversion");
    }
    return index;
}

int RootTask::get_num_axioms() const {
    return axioms.size();
}

int RootTask::get_num_goals() const {
    return goals.size();
}

FactPair RootTask::get_goal_fact(int index) const {
    assert(utils::in_bounds(index, goals));
    return goals[index];
}

vector<int> RootTask::get_initial_state_values() const {
    return initial_state_values;
}

void RootTask::convert_ancestor_state_values(
    vector<int> &, const AbstractTask *ancestor_task) const {
    if (this != ancestor_task) {
        ABORT("Invalid state conversion");
    }
}

void read_root_task(istream &in) {
    assert(!g_root_task);
    g_root_task = make_shared<RootTask>(in);
}

class RootTaskFeature
    : public plugins::TypedFeature<AbstractTask, AbstractTask> {
public:
    RootTaskFeature() : TypedFeature("no_transform") {
    }

    virtual shared_ptr<AbstractTask> create_component(const plugins::Options &, const utils::Context &) const override {
        return g_root_task;
    }
};

static plugins::FeaturePlugin<RootTaskFeature> _plugin;
}
