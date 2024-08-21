from typing import List, Union

from normalize import convert_to_DNF
from pddl.conditions import Condition, Disjunction, Conjunction
from pddl.actions import Action, PropositionalAction

def merge(actions: List[Union[Action, PropositionalAction]]):
    # Check if actions have same effects (on relevant vars)!
    #  - either pass relevant_vars as input to merge()
    #  - or scope the actions so they don't include irrelevant vars before sending to merge()
    h0 = actions[0].effect_hash()
    for a in actions[1:]:
        h = a.effect_hash()
        assert h == h0, 'Attempted to merge skills with different effects'
    if isinstance(actions[0], PropositionalAction):
        as_conjunction = lambda precond: Conjunction(precond)
    else:
        as_conjunction = lambda precond: precond
    formula_dnf = Disjunction([as_conjunction(a.precondition) for a in actions])
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
    print('ϕ')
    formula.dump()
    # 1. negate ϕ => ¬ϕ
    negated_formula = fully_simplify(formula.negate())
    print('¬ϕ')
    negated_formula.dump()
    # 2. convert ¬ϕ to DNF
    negated_DNF = fully_simplify(convert_to_DNF(negated_formula))
    print('(¬ϕ)_DNF')
    negated_DNF.dump()
    # 3. negate result and simplify
    cnf = fully_simplify(negated_DNF.negate())
    print('CNF')
    cnf.dump()
    return cnf
