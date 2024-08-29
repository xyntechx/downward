%cd ~/dev/downward/src/translate

#%%
import itertools
from typing import Dict, List, Set, Tuple, Union
import sys

import pddl_parser
from sas_tasks import VarValPair

domain_filename = '../../../scoping/domains/propositional/toy-minecraft/toy-example.pddl'
task_filename = '../../../scoping/domains/propositional/toy-minecraft/example-1.pddl'
sys.argv.extend([domain_filename, task_filename])
from translate import pddl_to_sas

from pddl.actions import VarValAction
from scoping.merging import merge

task = pddl_parser.open()
sas_task = pddl_to_sas(task)
sas_task.dump()

actions = sorted(
    [VarValAction.from_sas(op) for op in sas_task.operators],
    key = lambda a: a.name,
)
variable_domains = {i: set(range(r)) for i, r in enumerate(sas_task.variables.ranges)}

hunt_stv = actions[4]
gather_stv = actions[1]
actions = [hunt_stv, gather_stv]

#%% ---------------------

variable_domains : Dict[str, Set] = {'A': {0, 1, 2}, 'B': {0, 1, 2}, 'C': {0, 1}, 'D': {0, 1}}
preconds = [
    {        'B': 2, 'C': 1},
    {'A': 1, 'B': 2},
    {'A': 2, 'B': 2, 'C': 0},
    {'A': 0, 'B': 2, 'C': 0},
    {        'B': 0, 'C': 1},
    {'A': 1, 'B': 0},
    {'A': 2, 'B': 0, 'C': 0},
    {'A': 0, 'B': 0, 'C': 0},
    {'C': 0},
    # {'C': 1},
]
variables = [v for v in sorted(variable_domains.keys())]

def precond_to_varval_list(variables: List[str], precondition: Dict[str, int]) -> List[VarValPair]:
    return [(var, precondition[var]) for var in variables if var in precondition]

names = [f'a{i}' for i, _ in enumerate(preconds)]
precond_lists = [precond_to_varval_list(variables, precond) for precond in preconds]
dummy_effects = [[(i, 0) for i, _ in enumerate(variables)] for _ in preconds]
costs = [1 for _ in preconds]

actions = [
    VarValAction(name, pre, eff, cost)
    for name, pre, eff, cost in zip(names, precond_lists, dummy_effects, costs)
]

#%% ----------------------

def merge(actions: List[VarValAction], variable_domains: Dict[str, Set], output_info: bool = False) -> Union[List[str], Tuple[List[str], Dict]]:
    # collect the precondition variables
    action_precond_vars = [set([var for (var, _) in a.precondition]) for a in actions]
    all_precond_vars = set().union(*action_precond_vars)

    # identify which precond vars are don't-cares for each action
    action_dc_vars : List[Set] = [all_precond_vars.difference(vars) for vars in action_precond_vars]

    # build the set of satisfying partial states for the merged action
    satisfying_partial_states : Set[Tuple] = set()
    for action_id, action in enumerate(actions):
        dc_vars = [(var, variable_domains[var]) for var in action_dc_vars[action_id]]
        dc_val_possibilities = [values for (_, values) in dc_vars]
        dc_val_combinations = list(itertools.product(*dc_val_possibilities))
        dc_varval_combinations = [[(var, val) for (var, _), val in zip(dc_vars, values)] for values in dc_val_combinations]

        [satisfying_partial_states.add(tuple(sorted(action.precondition + dcs))) for dcs in dc_varval_combinations]

    # see if any variables can be removed without changing the satisfying set
    removed_vars = []
    for removed_var in all_precond_vars:
        partial_states_without_var = {tuple((var, val) for var, val in partial_state if var != removed_var) for partial_state in satisfying_partial_states}

        if len(partial_states_without_var) * len(variable_domains[removed_var]) == len(satisfying_partial_states):
            satisfying_partial_states = partial_states_without_var
            removed_vars.append(removed_var)

    info = {'satisfying_partial_states': satisfying_partial_states}

    # return relevant precondition vars
    relevant_precond_vars = [var for var in all_precond_vars if var not in removed_vars]
    if output_info:
        return relevant_precond_vars, satisfying_partial_states
    else:
        return relevant_precond_vars

merge(actions, variable_domains, output_info=True)
