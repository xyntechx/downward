#include "min_open_list.h"

#include "../open_list.h"

#include "../plugins/plugin.h"
#include "../utils/memory.h"
#include "../utils/system.h"

#include <cassert>
#include <memory>
#include <vector>
#include <optional>

using namespace std;
using utils::ExitCode;

namespace min_open_list {
template<class Entry>
class MinOpenList : public OpenList<Entry> {
    vector<unique_ptr<OpenList<Entry>>> open_lists;
    vector<int> priorities;
    vector<std::optional<Entry>> s_ids;

    const int boost_amount;
protected:
    virtual void do_insertion(EvaluationContext &eval_context, const Entry &entry) override;
    virtual void do_insertion(EvaluationContext &eval_context, const Entry &entry, const std::string op_name) override;

public:
    MinOpenList(
        const vector<shared_ptr<OpenListFactory>> &sublists, int boost);

    virtual Entry remove_min() override;
    virtual bool empty() const override;
    virtual void clear() override;
    virtual void boost_preferred() override;
    virtual void get_path_dependent_evaluators(
        set<Evaluator *> &evals) override;
    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override;
    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override;
};


template<class Entry>
MinOpenList<Entry>::MinOpenList(
    const vector<shared_ptr<OpenListFactory>> &sublists, int boost)
    : boost_amount(boost) {
    vector<shared_ptr<OpenListFactory>> open_list_factories(sublists);
    open_lists.reserve(open_list_factories.size());
    for (const auto &factory : open_list_factories)
        open_lists.push_back(factory->create_open_list<Entry>());

    priorities.resize(open_lists.size(), 0);
    s_ids.resize(open_lists.size());
}

template<class Entry>
void MinOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry) {
    for (const auto &sublist : open_lists)
        sublist->insert(eval_context, entry);
}

template<class Entry>
void MinOpenList<Entry>::do_insertion(
    EvaluationContext &eval_context, const Entry &entry, const std::string op_name) {
    for (const auto &sublist : open_lists)
        sublist->insert(eval_context, entry, op_name);

    // for (size_t i = 0; i < open_lists.size(); ++i) {
    //     open_lists[i]->insert(eval_context, entry, op_name);
    // }

    // int idx = op_name.find("macro") == std::string::npos ? 0 : 1;
    // if (!s_ids[idx].has_value()) {
    //     if (idx >= open_lists.size() || !open_lists[idx] || open_lists[idx]->empty()) {
    //         return;
    //     }
    //     std::vector<std::any> res = open_lists[idx]->remove_min_complete();
    //     priorities[idx] = std::any_cast<int>(res[0]);
    //     s_ids[idx] = std::any_cast<Entry>(res[1]);
    // }
}

template<class Entry>
Entry MinOpenList<Entry>::remove_min() {
    int best = -1;
    for (size_t i = 0; i < open_lists.size(); ++i) {
        if (!open_lists[i]->empty() &&
        // if (s_ids[i].has_value() &&
            (best == -1 || priorities[i] < priorities[best])) {
            best = i;
        }
    }
    assert(best != -1);
    const auto &best_list = open_lists[best];
    assert(!best_list->empty());

    Entry result = *s_ids[best];

    std::vector<std::any> res = best_list->remove_min_complete();
    priorities[best] = std::any_cast<int>(res[0]);
    s_ids[best] = std::any_cast<Entry>(res[1]);

    return result;
}

template<class Entry>
bool MinOpenList<Entry>::empty() const {
    for (const auto &sublist : open_lists)
        if (!sublist->empty())
            return false;
    return true;
}

template<class Entry>
void MinOpenList<Entry>::clear() {
    for (const auto &sublist : open_lists)
        sublist->clear();
}

template<class Entry>
void MinOpenList<Entry>::boost_preferred() {
    for (size_t i = 0; i < open_lists.size(); ++i)
        if (open_lists[i]->only_contains_preferred_entries())
            priorities[i] -= boost_amount;
}

template<class Entry>
void MinOpenList<Entry>::get_path_dependent_evaluators(
    set<Evaluator *> &evals) {
    for (const auto &sublist : open_lists)
        sublist->get_path_dependent_evaluators(evals);
}

template<class Entry>
bool MinOpenList<Entry>::is_dead_end(
    EvaluationContext &eval_context) const {
    // If one sublist is sure we have a dead end, return true.
    if (is_reliable_dead_end(eval_context))
        return true;
    // Otherwise, return true if all sublists agree this is a dead-end.
    for (const auto &sublist : open_lists)
        if (!sublist->is_dead_end(eval_context))
            return false;
    return true;
}

template<class Entry>
bool MinOpenList<Entry>::is_reliable_dead_end(
    EvaluationContext &eval_context) const {
    for (const auto &sublist : open_lists)
        if (sublist->is_reliable_dead_end(eval_context))
            return true;
    return false;
}


MinOpenListFactory::MinOpenListFactory(
    const vector<shared_ptr<OpenListFactory>> &sublists, int boost)
    : sublists(sublists),
      boost(boost) {
}

unique_ptr<StateOpenList>
MinOpenListFactory::create_state_open_list() {
    return utils::make_unique_ptr<MinOpenList<StateOpenListEntry>>(
        sublists, boost);
}

unique_ptr<EdgeOpenList>
MinOpenListFactory::create_edge_open_list() {
    return utils::make_unique_ptr<MinOpenList<EdgeOpenListEntry>>(
        sublists, boost);
}

class MinOpenListFeature
    : public plugins::TypedFeature<OpenListFactory, MinOpenListFactory> {
public:
    MinOpenListFeature() : TypedFeature("minol") {
        document_title("Min open list");
        document_synopsis(
            "chooses the sub open list with the minimum min.");

        add_list_option<shared_ptr<OpenListFactory>>(
            "sublists",
            "open lists between which this one chooses");
        add_option<int>(
            "boost",
            "boost value for contained open lists that are restricted "
            "to preferred successors",
            "0");
    }

    virtual shared_ptr<MinOpenListFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<shared_ptr<OpenListFactory>>(
            context, opts, "sublists");
        return plugins::make_shared_from_arg_tuples<MinOpenListFactory>(
            opts.get_list<shared_ptr<OpenListFactory>>("sublists"),
            opts.get<int>("boost")
            );
    }
};

static plugins::FeaturePlugin<MinOpenListFeature> _plugin;
}
