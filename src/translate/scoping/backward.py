# %%
#!%cd ~/dev/downward/src/translate

# %%
from typing import Any, Tuple

import options
import pddl_parser
from pddl.actions import VarValAction
from sas_tasks import SASTask, VarValPair
from scoping.factset import FactSet
from scoping.merging import merge
from scoping.sas_parser import SasParser
from translate import pddl_to_sas

# %%
# domain_filename = "../../../scoping/domains/propositional/ipc/gripper/domain.pddl"
# task_filename = "../../../scoping/domains/propositional/ipc/gripper/prob04.pddl"

domain_filename = (
    "../../../scoping/domains/propositional/toy-minecraft/toy-example.pddl"
)
task_filename = "../../../scoping/domains/propositional/toy-minecraft/example-2.pddl"
options.keep_unimportant_variables = True
options.keep_unreachable_facts = True
options.sas_file = True
task = pddl_parser.open(domain_filename, task_filename)
sas_task: SASTask = pddl_to_sas(task)

# %%
# sas_path = "../../toy-minecraft-merging.sas"
# parser = SasParser(pth=sas_path)
# parser.parse()
# sas_task: SASTask = parser.to_fd()

# %%


def filter_causal_links(
    facts: FactSet, init: list[VarValPair], actions: set[VarValAction]
) -> FactSet:
    """Remove any facts from `facts` that are present in the initial state `init` and
    unthreatened by any of the `actions`."""
    affected_facts = FactSet()
    for a in actions:
        affected_facts.add(a.effects)
    unthreatened_init_facts = [
        (var, val) for (var, val) in init if affected_facts[var] in [set(), set([val])]
    ]
    unthreatened_init_facts = FactSet(unthreatened_init_facts)
    relevant_facts = FactSet()
    for var, values in facts:
        for val in values:
            if (var, val) not in unthreatened_init_facts:
                relevant_facts.add(var, val)
    return relevant_facts


def get_goal_relevant_actions(
    facts: FactSet, actions: list[VarValAction]
) -> list[VarValAction]:
    """Find all actions that achieve at least one fact in `facts`."""
    # The same action may achieve multiple facts, so we de-duplicate with a set
    return list(set([a for a in actions for fact in a.effects if fact in facts]))


def partition_actions(
    relevant_variables: list[Any], actions: list[VarValAction]
) -> list[list[VarValAction]]:
    """Partition actions by (effect, cost), ignoring irrelevant variables"""
    unique_effects_and_costs = set([a.effect_hash(relevant_variables) for a in actions])
    effect_cost_partitions = []
    for effect_cost in unique_effects_and_costs:
        matching_actions = [
            a for a in actions if a.effect_hash(relevant_variables) == effect_cost
        ]
        effect_cost_partitions.append(matching_actions)
    return effect_cost_partitions


def get_goal_relevant_facts(
    domains: FactSet,
    relevant_facts: FactSet,
    relevant_actions: list[VarValAction],
    enable_merging: bool = False,
) -> FactSet:
    """Find all facts that appear in the (simplified) preconditions
    of the (possibly merged) relevant actions."""
    relevant_vars = relevant_facts.variables
    if enable_merging:
        action_partitions = partition_actions(relevant_vars, relevant_actions)
    else:
        # make an separate partition for each action
        action_partitions = list(map(lambda x: [x], relevant_actions))

    relevant_facts = FactSet()
    for actions in action_partitions:
        relevant_precond_facts = merge(actions, relevant_vars, domains)
        relevant_facts.union(relevant_precond_facts)

    return relevant_facts


def goal_relevance_step(
    domains: FactSet,
    facts: FactSet,
    init: list[VarValPair],
    actions: list[VarValAction],
    relevant_actions: list[VarValAction],
    enable_merging: bool = False,
    enable_causal_links: bool = False,
) -> Tuple[FactSet, list[VarValAction]]:
    if enable_causal_links:
        filtered_facts = filter_causal_links(facts, init, relevant_actions)
    else:
        filtered_facts = facts
    relevant_actions = get_goal_relevant_actions(filtered_facts, actions)
    relevant_facts = get_goal_relevant_facts(
        domains,
        filtered_facts,
        relevant_actions,
        enable_merging=enable_merging,
    )
    relevant_facts.union(filtered_facts)

    return relevant_facts, relevant_actions


def compute_goal_relevance(
    sas_task: SASTask,
    enable_merging: bool = False,
    enable_causal_links: bool = False,
) -> Tuple[FactSet, list[VarValAction]]:
    domains = FactSet(
        {i: set(range(r)) for i, r in enumerate(sas_task.variables.ranges)}
    )
    init = list(enumerate(sas_task.init.values))
    actions = [VarValAction.from_sas(op) for op in sas_task.operators]
    relevant_facts = FactSet(sas_task.goal.pairs)
    relevant_actions = []
    prev_facts = None
    prev_actions = []
    while relevant_facts != prev_facts or len(relevant_actions) != len(prev_actions):
        prev_facts, prev_actions = relevant_facts, relevant_actions
        relevant_facts, relevant_actions = goal_relevance_step(
            domains,
            relevant_facts,
            init,
            actions,
            relevant_actions,
            enable_merging,
            enable_causal_links,
        )

    return relevant_facts, relevant_actions


# %%

# sas_task.dump()

for merging in [False, True]:
    for causal_links in [False, True]:
        facts, actions = compute_goal_relevance(
            sas_task,
            enable_merging=merging,
            enable_causal_links=causal_links,
        )
        print(f"{merging=}")
        print(f"{causal_links=}")
        print("actions:", len(actions), sorted(a.name for a in actions))
        print("facts:", sorted(facts))
        print()
