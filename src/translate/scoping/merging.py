import itertools
from typing import Dict, List, Set, Tuple, Union, Optional

from normalize import convert_to_DNF
from pddl.conditions import Condition, Disjunction, Conjunction
from pddl.actions import Action, PropositionalAction, VarValAction
from scoping.factset import FactSet


def precondition_facts(action: VarValAction, variable_domains: FactSet) -> FactSet:
    precond_facts = FactSet()
    for var, val in action.precondition:
        if val == -1:
            precond_facts.union(var, variable_domains[var])
        else:
            precond_facts.add(var, val)
    return precond_facts


def merge(
    actions: Set[VarValAction],
    variable_domains: FactSet,
    output_info: bool = False,
    mode: str = "variables",
) -> Tuple[Union[List[str], FactSet], Optional[Dict]]:
    actions: List[VarValAction] = list(actions)
    if len(actions) == 1:
        return precondition_facts(actions[0], variable_domains)
    h0 = actions[0].hashable()
    for a in actions[1:]:
        h = a.hashable()
        assert h == h0, "Attempted to merge skills with different effects/costs"

    # # TODO: Is merging only useful if at least one variable spans its whole domain?
    # precond_facts = FactSet()
    # for a in actions:
    #     precond_facts.union(precondition_facts(a, variable_domains))
    # if not any(values == variable_domains[var] for var, values in precond_facts):
    #     return precond_facts

    # collect the precondition variables
    action_precond_vars = [set([var for (var, _) in a.precondition]) for a in actions]
    all_precond_vars = set().union(*action_precond_vars)

    # identify which precond vars are don't-cares for each action
    action_dc_vars: List[Set] = [
        all_precond_vars.difference(vars) for vars in action_precond_vars
    ]

    # build the set of satisfying partial states for the merged action
    satisfying_partial_states: Set[Tuple] = set()
    for action_id, action in enumerate(actions):
        dc_vars = [(var, variable_domains[var]) for var in action_dc_vars[action_id]]
        dc_val_possibilities = [values for (_, values) in dc_vars]
        dc_val_combinations = list(itertools.product(*dc_val_possibilities))
        dc_varval_combinations = [
            [(var, val) for (var, _), val in zip(dc_vars, values)]
            for values in dc_val_combinations
        ]

        [
            satisfying_partial_states.add(tuple(sorted(action.precondition + dcs)))
            for dcs in dc_varval_combinations
        ]

    # see if any variables can be removed without changing the satisfying set
    removed_vars = []
    for removed_var in all_precond_vars:
        partial_states_without_var = {
            tuple((var, val) for var, val in partial_state if var != removed_var)
            for partial_state in satisfying_partial_states
        }

        if len(partial_states_without_var) * len(variable_domains[removed_var]) == len(
            satisfying_partial_states
        ):
            satisfying_partial_states = partial_states_without_var
            removed_vars.append(removed_var)

    # return relevant precondition vars
    relevant_precond_vars = [var for var in all_precond_vars if var not in removed_vars]
    relevant_precond_facts = FactSet()
    for partial_state in satisfying_partial_states:
        relevant_precond_facts.add(partial_state)

    result = relevant_precond_vars if mode == "variables" else relevant_precond_facts
    if output_info:
        result = (result, satisfying_partial_states)
    return result


def merge_pddl(actions: List[Union[Action, PropositionalAction]]):
    # Check if actions have same effects (on relevant vars)!
    #  - either pass relevant_vars as input to merge()
    #  - or scope the actions so they don't include irrelevant vars before sending to merge()
    h0 = actions[0].hashable()
    for a in actions[1:]:
        h = a.hashable()
        assert h == h0, "Attempted to merge skills with different effects"
    if isinstance(actions[0], PropositionalAction):
        get_precond = lambda action: Conjunction(action.precondition)
    else:
        get_precond = lambda action: action.precondition
    formula_dnf = Disjunction([get_precond(a) for a in actions])
    formula_cnf = dnf2cnf(formula_dnf)
    return formula_cnf, formula_dnf


def fully_simplify(formula: Condition, max_iters: int = 10):
    result = formula
    for _ in range(max_iters):
        simplified = result.simplified()
        if simplified == result:
            break
        else:
            result = simplified
    return result


def dnf2cnf(formula: Disjunction):
    """Convert formula ϕ to CNF"""
    formula = fully_simplify(formula)
    # print('ϕ')
    # formula.dump()

    # 1. negate ϕ => ¬ϕ
    negated_formula = fully_simplify(formula.negate())
    # print('¬ϕ')
    # negated_formula.dump()

    # 2. convert ¬ϕ to DNF
    negated_DNF = fully_simplify(convert_to_DNF(negated_formula))
    # print('(¬ϕ)_DNF')
    # negated_DNF.dump()

    # 3. negate result and simplify
    cnf = fully_simplify(negated_DNF.negate())
    # print('CNF')
    # cnf.dump()

    return cnf
