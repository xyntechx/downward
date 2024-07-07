from normalize import convert_to_DNF
from pddl.conditions import Condition, Disjunction

def merge(actions):
    formula_dnf = Disjunction([a.precondition for a in actions])
    formula_cnf = dnf2cnf(formula_dnf)
    return formula_cnf, formula_dnf

def fully_simplify(formula: Condition):
    result = formula
    for _ in range(10):
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
