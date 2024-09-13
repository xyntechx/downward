from collections import defaultdict
from typing import Any, Dict, List, Optional, overload, Set, Tuple, Union


class FactSet:
    facts: Dict[Any, Set[Any]]

    def __init__(
        self,
        facts: Union[
            "FactSet", Dict[Any, Set[Any]], List[Tuple[Any, Any]], None
        ] = None,
    ) -> None:
        self.facts = defaultdict(set)
        if facts is None:
            return
        if isinstance(facts, (FactSet, dict)):
            self.union(facts)
        else:
            self.add(facts)

    def __repr__(self) -> str:
        return f"FactSet({repr(dict(self.facts))})"

    def __getitem__(self, key: Any) -> Set[Any]:
        return self.facts[key]

    def __eq__(self, other: Optional["FactSet"]) -> bool:
        if other is None:
            return False
        return self.facts == other.facts

    def __len__(self) -> int:
        return len(self.facts)

    def __iter__(self):
        return iter(self.facts.items())

    def keys(self):
        return self.facts.keys()

    def values(self):
        return self.facts.values()

    def items(self):
        return self.facts.items()

    @overload
    def add(self, var: Any, val: Any) -> None: ...
    @overload
    def add(self, fact_list: List[Tuple[Any, Any]]) -> None: ...
    def add(
        self,
        fact_list_or_var: Union[Any, List[Tuple[Any, Any]]],
        val: Optional[Any] = None,
    ) -> None:
        """Add a new fact (var = val), or a list of such facts, to the FactSet"""
        if val is None:
            fact_list = fact_list_or_var
            for var, val in fact_list:
                self.add(var, val)
        else:
            var = fact_list_or_var
            self.facts[var].add(val)

    @overload
    def union(self, other_facts: "FactSet") -> None: ...
    @overload
    def union(self, var: Any, values: Set[Any]) -> None: ...
    def union(
        self,
        other_facts_or_var: Union["FactSet", Any],
        values: Optional[Set[Any]] = None,
    ) -> None:
        """Take the in-place union of the FactSet with the specified additional facts"""
        if values is None:
            other_facts = other_facts_or_var
            for var, values in other_facts.items():
                self.union(var, values)
        else:
            var = other_facts_or_var
            self.facts[var] = self.facts[var].union(values)

    def __contains__(self, item) -> bool:
        var, val = item
        if var not in self.facts:
            return False
        values = self.facts[var]
        return val in values
