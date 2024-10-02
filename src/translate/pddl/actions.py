import copy
from typing import Any, List, Optional, Tuple

from . import conditions
from .conditions import Condition, Literal
from sas_tasks import SASOperator, VarValPair
from .effects import Effect
from .f_expression import Increase
from .pddl_types import TypedObject
from scoping.factset import FactSet


class Action:
    def __init__(
        self,
        name: str,
        parameters: List[TypedObject],
        num_external_parameters: int,
        precondition: Condition,
        effects: List[Effect],
        cost: Optional[Increase],
    ):
        assert 0 <= num_external_parameters <= len(parameters)
        self.name = name
        self.parameters = parameters
        # num_external_parameters denotes how many of the parameters
        # are "external", i.e., should be part of the grounded action
        # name. Usually all parameters are external, but "invisible"
        # parameters can be created when compiling away existential
        # quantifiers in conditions.
        self.num_external_parameters = num_external_parameters
        self.precondition = precondition
        self.effects = effects
        self.cost = cost
        self.uniquify_variables()  # TODO: uniquify variables in cost?

    def __repr__(self):
        return "<Action %r at %#x>" % (self.name, id(self))

    def hashable(self):
        return tuple(self.effects), self.cost

    def dump(self):
        print("%s(%s)" % (self.name, ", ".join(map(str, self.parameters))))
        print("Precondition:")
        self.precondition.dump()
        print("Effects:")
        for eff in self.effects:
            eff.dump()
        print("Cost:")
        if self.cost:
            self.cost.dump()
        else:
            print("  None")

    def uniquify_variables(self):
        self.type_map = {par.name: par.type_name for par in self.parameters}
        self.precondition = self.precondition.uniquify_variables(self.type_map)
        for effect in self.effects:
            effect.uniquify_variables(self.type_map)

    def relaxed(self):
        new_effects = []
        for eff in self.effects:
            relaxed_eff = eff.relaxed()
            if relaxed_eff:
                new_effects.append(relaxed_eff)
        return Action(
            self.name,
            self.parameters,
            self.num_external_parameters,
            self.precondition.relaxed().simplified(),
            new_effects,
        )

    def untyped(self):
        # We do not actually remove the types from the parameter lists,
        # just additionally incorporate them into the conditions.
        # Maybe not very nice.
        result = copy.copy(self)
        parameter_atoms = [par.to_untyped_strips() for par in self.parameters]
        new_precondition = self.precondition.untyped()
        result.precondition = conditions.Conjunction(
            parameter_atoms + [new_precondition]
        )
        result.effects = [eff.untyped() for eff in self.effects]
        return result

    def instantiate(
        self,
        var_mapping,
        init_facts,
        init_assignments,
        fluent_facts,
        objects_by_type,
        metric,
    ):
        """Return a PropositionalAction which corresponds to the instantiation of
        this action with the arguments in var_mapping. Only fluent parts of the
        conditions (those in fluent_facts) are included. init_facts are evaluated
        while instantiating.
        Precondition and effect conditions must be normalized for this to work.
        Returns None if var_mapping does not correspond to a valid instantiation
        (because it has impossible preconditions or an empty effect list.)"""
        arg_list = [
            var_mapping[par.name]
            for par in self.parameters[: self.num_external_parameters]
        ]
        name = "(%s %s)" % (self.name, " ".join(arg_list))

        precondition = []
        try:
            self.precondition.instantiate(
                var_mapping, init_facts, fluent_facts, precondition
            )
        except conditions.Impossible:
            return None
        effects = []
        for eff in self.effects:
            eff.instantiate(
                var_mapping, init_facts, fluent_facts, objects_by_type, effects
            )
        if effects:
            if metric:
                if self.cost is None:
                    cost = 0
                else:
                    cost = int(
                        self.cost.instantiate(
                            var_mapping, init_assignments
                        ).expression.value
                    )
            else:
                cost = 1
            return PropositionalAction(name, precondition, effects, cost)
        else:
            return None


class PropositionalAction:
    def __init__(
        self,
        name: str,
        precondition: List[Literal],
        effects: List[Tuple[List[Literal], Literal]],
        cost: int,
    ):
        self.name = name
        self.precondition = precondition
        self.add_effects = []
        self.del_effects = []
        for condition, effect in effects:
            if not effect.negated:
                self.add_effects.append((condition, effect))
        # Warning: This is O(N^2), could be turned into O(N).
        # But that might actually harm performance, since there are
        # usually few effects.
        # TODO: Measure this in critical domains, then use sets if acceptable.
        for condition, effect in effects:
            if effect.negated and (condition, effect.negate()) not in self.add_effects:
                self.del_effects.append((condition, effect.negate()))
        self.cost = cost

    def __repr__(self):
        return "<PropositionalAction %r at %#x>" % (self.name, id(self))

    def hashable(self):
        def make_tuple(effects):
            return tuple((tuple(c), e) for c, e in effects)

        return (make_tuple(self.add_effects), make_tuple(self.del_effects)), self.cost

    def dump(self):
        print(self.name)
        for fact in self.precondition:
            print("PRE: %s" % fact)
        for cond, fact in self.add_effects:
            print("ADD: %s -> %s" % (", ".join(map(str, cond)), fact))
        for cond, fact in self.del_effects:
            print("DEL: %s -> %s" % (", ".join(map(str, cond)), fact))
        print("cost:", self.cost)


class VarValAction:
    def __init__(
        self,
        name: str,
        precondition: List[VarValPair],
        effect: List[VarValPair],
        cost: int,
    ):
        self.name = name
        self.precondition = precondition
        self.effects = effect
        self.cost = cost

    @classmethod
    def from_sas(cls, sas_operator: SASOperator):
        assert not any(
            [cond for (_, _, _, cond) in sas_operator.pre_post]
        ), "Conditional effects not implemented"
        pre_list = [
            (var, pre) for (var, pre, _, _) in sas_operator.pre_post if pre != -1
        ]
        pre_list += sas_operator.prevail
        eff_list = [(var, post) for (var, pre, post, conds) in sas_operator.pre_post]
        # TODO: remove duplicates?
        pre_list = sorted(list(set(pre_list)))
        return cls(sas_operator.name, pre_list, eff_list, sas_operator.cost)

    @property
    def prevail(self) -> list[VarValPair]:
        effect_facts = FactSet(self.effects)

        def is_prevail(var_val: VarValPair):
            if var_val not in self.precondition:
                return False
            var, val = var_val
            if var not in effect_facts.variables:
                return True
            if set([val]) != effect_facts[var]:
                return False
            return True

        return [fact for fact in self.precondition if is_prevail(fact)]

    @property
    def pre_post(self) -> List[Tuple[int, int, int, List[VarValPair]]]:
        prevails = self.prevail
        precond_facts = FactSet(
            [fact for fact in self.precondition if fact not in prevails]
        )

        def get_precond(var):
            if precond_facts[var]:
                return precond_facts[var].pop()
            return -1

        return [
            (var, get_precond(var), val, [])
            for var, val in self.effects
            if (var, val) not in prevails
        ]

    def __eq__(self, other: object) -> bool:
        if self.name != other.name:
            return False
        if self.precondition != other.precondition:
            return False
        if self.effects != other.effects:
            return False
        if self.cost != other.cost:
            return False
        return True

    def __repr__(self):
        return f"VarValAction({self.name}, pre={self.precondition}, eff={self.effects})"

    def __hash__(self) -> int:
        return hash(
            (self.name, tuple(self.precondition), tuple(self.effects), self.cost)
        )

    def effect_hash(
        self, relevant_variables: List[Any]
    ) -> Tuple[List[VarValPair], int]:
        return tuple(
            [(var, val) for (var, val) in self.effects if var in relevant_variables]
        ), self.cost

    def dump(self):
        print(self.name)
        for fact in self.precondition:
            print(f"PRE: {fact}")
        for fact in self.effects:
            print(f"EFF: {fact}")
        print("cost:", self.cost)
