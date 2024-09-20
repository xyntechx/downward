# %%
#!%cd ~/dev/downward/src/translate

# %%
from typing import Any, List, Set, Tuple

import pddl_parser

from sas_tasks import SASTask
from pddl.actions import VarValAction
import options
from scoping.factset import FactSet
from scoping.merging import merge
from scoping.sas_parser import SasParser
from translate import pddl_to_sas

# %%
# domain_filename = "../../../scoping/domains/propositional/ipc/gripper/domain.pddl"
# task_filename = "../../../scoping/domains/propositional/ipc/gripper/prob04.pddl"

# domain_filename = (
#     "../../../scoping/domains/propositional/toy-minecraft/toy-example.pddl"
# )
# task_filename = "../../../scoping/domains/propositional/toy-minecraft/example-1.pddl"
# options.keep_unimportant_variables = True
# options.sas_file = True
# task = pddl_parser.open(domain_filename, task_filename)
# sas_task: SASTask = pddl_to_sas(task)

# sas_task.dump()

# %%
sas_path = "../../toy-minecraft.sas"
parser = SasParser(pth=sas_path)
parser.parse()
sas_task: SASTask = parser.to_fd()


def filter_unthreatened_causal_links(
    facts: FactSet, init: List[Tuple[Any, Any]], actions: Set[VarValAction]
) -> FactSet:
    affected_facts = FactSet()
    for a in actions:
        affected_facts.add(a.effects)
    unthreatened_init_facts = FactSet(
        [(var, val) for (var, val) in init if affected_facts[var] in [set(), set(val)]]
    )
    relevant_facts = FactSet()
    for var, values in facts:
        for val in values:
            if (var, val) not in unthreatened_init_facts:
                relevant_facts.add((var, val))
    return relevant_facts


def get_backward_reachable_actions(
    facts: FactSet, actions: Set[VarValAction]
) -> Set[VarValAction]:
    return set([a for a in actions for fact in a.effects if fact in facts])


def get_backward_reachable_facts(
    variable_domains: FactSet,
    reachable_actions: List[VarValAction],
    init: List[Tuple[Any, Any]],
    enable_merging: bool = False,
    enable_causal_links: bool = False,
) -> FactSet:
    if enable_merging:
        # partition actions by (effect, cost)
        unique_effects_and_costs = [a.hashable() for a in reachable_actions]
        effect_cost_partitions = {
            effect_cost: set(
                [a for a in reachable_actions if a.hashable() == effect_cost]
            )
            for effect_cost in unique_effects_and_costs
        }
        # merge the actions in each partition
        reachable_factsets = [
            merge(action_list, variable_domains, mode="values")
            for action_list in effect_cost_partitions.values()
        ]
        reachable_facts = FactSet()
        for partition_factset in reachable_factsets:
            reachable_facts.union(partition_factset)
    else:
        reachable_facts = FactSet()
        for action in reachable_actions:
            for var, val in action.precondition:
                if val == -1:
                    reachable_facts.union(var, variable_domains[var])
                else:
                    reachable_facts.add(var, val)

    # if not enable_causal_links:
    #     relevant_facts = reachable_facts
    # else:
    #     affected_facts = FactSet()
    #     for a in reachable_actions:
    #         affected_facts.add(a.effects)
    #     unthreatened_init_facts = FactSet(
    #         [
    #             (var, val)
    #             for (var, val) in init
    #             if affected_facts[var] in [set(), set([val])]
    #         ]
    #     )
    #     relevant_facts = FactSet()
    #     for var, values in reachable_facts:
    #         for val in values:
    #             if (var, val) not in unthreatened_init_facts:
    #                 relevant_facts.add(var, val)

    return reachable_facts


def backward_reachability_step(
    variable_domains: FactSet,
    facts: FactSet,
    init: List[Tuple[Any, Any]],
    actions: Set[VarValAction],
    enable_merging: bool = False,
    enable_causal_links: bool = False,
) -> Tuple[FactSet, Set[VarValAction]]:
    # filtered_facts = filter_unthreatened_causal_links(facts, init, actions)
    reachable_actions = get_backward_reachable_actions(facts, actions)
    reachable_facts = get_backward_reachable_facts(
        variable_domains,
        reachable_actions,
        init,
        enable_merging=enable_merging,
        enable_causal_links=enable_causal_links,
    )
    reachable_facts.union(facts)

    return reachable_facts, reachable_actions


def backward_reachability(
    sas_task: SASTask,
    enable_merging: bool = False,
    enable_causal_links: bool = False,
) -> Tuple[FactSet, List[VarValAction]]:
    actions = set([VarValAction.from_sas(op) for op in sas_task.operators])
    variable_domains = FactSet(
        {i: set(range(r)) for i, r in enumerate(sas_task.variables.ranges)}
    )
    init = list(enumerate(sas_task.init.values))
    facts = FactSet(sas_task.goal.pairs)
    prev_facts = None
    while facts != prev_facts:
        prev_facts = facts
        facts, reachable_actions = backward_reachability_step(
            variable_domains, facts, init, actions, enable_merging, enable_causal_links
        )
        facts

    return facts, reachable_actions


# %%
for merging in [False, True]:
    for causal_links in [False, True]:
        facts, actions = backward_reachability(
            sas_task,
            enable_merging=merging,
            enable_causal_links=causal_links,
        )
        print(f"{merging=}, {causal_links=}")
        print("actions:", len(actions), sorted(a.name for a in actions))
        print("facts:", sorted(facts))
        print()
