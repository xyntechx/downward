#include "macro_gen_open_list.h"

#include "../evaluator.h"
#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/memory.h"

#include <cassert>
#include <deque>
#include <map>

using namespace std;

namespace macro_gen_open_list {
template<class Entry>
class MacroGenOpenList : public OpenList<Entry> {
    typedef deque<Entry> Bucket;

    map<int, Bucket> buckets;
    int size;

    shared_ptr<Evaluator> evaluator;

protected:
    virtual void do_insertion(EvaluationContext &eval_context, const Entry &entry, const std::string op_name) override;

public:
    MacroGenOpenList(const shared_ptr<Evaluator> &eval, bool preferred_only);

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void get_path_dependent_evaluators(set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};

template<class Entry>
MacroGenOpenList<Entry>::MacroGenOpenList(
    const shared_ptr<Evaluator> &evaluator, bool preferred_only)
    : OpenList<Entry>(preferred_only),
      size(0),
      evaluator(evaluator) {
}

template<class Entry>
void MacroGenOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry, const std::string op_name) {
    if (op_name.find("macro") == std::string::npos) {
        // if operator name does NOT contain "macro"
        return;
    }
    int key = eval_context.get_evaluator_value(evaluator.get());
    buckets[key].push_back(entry);
    ++size;
}

template<class Entry>
Entry MacroGenOpenList<Entry>::remove_min() {
    assert(size > 0);
    auto it = buckets.begin();
    assert(it != buckets.end());
    Bucket &bucket = it->second;
    assert(!bucket.empty());
    Entry result = bucket.front();
    bucket.pop_front();
    if (bucket.empty())
        buckets.erase(it);
    --size;
    return result;
}

template<class Entry>
bool MacroGenOpenList<Entry>::empty() const {
    return size == 0;
}

template<class Entry>
void MacroGenOpenList<Entry>::clear() {
    buckets.clear();
    size = 0;
}

template<class Entry>
void MacroGenOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    evaluator->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool MacroGenOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    return eval_context.is_evaluator_value_infinite(evaluator.get());
}

template<class Entry>
bool MacroGenOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
}

MacroGenOpenListFactory::MacroGenOpenListFactory(
    const shared_ptr<Evaluator> &eval, bool pref_only)
    : eval(eval),
      pref_only(pref_only) {
}

unique_ptr<StateOpenList>
MacroGenOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<MacroGenOpenList<StateOpenListEntry>>(
        eval, pref_only);
}

unique_ptr<EdgeOpenList>
MacroGenOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<MacroGenOpenList<EdgeOpenListEntry>>(
        eval, pref_only);
}

class MacroGenOpenListFeature
    : public plugins::TypedFeature<OpenListFactory, MacroGenOpenListFactory> {
public:
    MacroGenOpenListFeature() : TypedFeature("mgen") {
        document_title("Focused macro-generated open list (derived from best-first)");
        document_synopsis(
            "Open list that uses a single evaluator and FIFO tiebreaking.");

        add_option<shared_ptr<Evaluator>>("eval", "evaluator");
        add_open_list_options_to_feature(*this);

        document_note(
            "Implementation Notes",
            "Elements with the same evaluator value are stored in double-ended "
            "queues, called \"buckets\". The open list stores a map from evaluator "
            "values to buckets. Pushing and popping from a bucket runs in constant "
            "time. Therefore, inserting and removing an entry from the open list "
            "takes time O(log(n)), where n is the number of buckets.");
    }


    virtual shared_ptr<MacroGenOpenListFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
        return plugins::make_shared_from_arg_tuples<MacroGenOpenListFactory>(
            opts.get<shared_ptr<Evaluator>>("eval"),
            get_open_list_arguments_from_options(opts));
    }
};

static plugins::FeaturePlugin<MacroGenOpenListFeature> _plugin;
}
