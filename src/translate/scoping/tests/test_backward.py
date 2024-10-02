#!%cd ~/dev/downward/src/translate
#
from scoping.backward import compute_goal_relevance
from scoping.factset import FactSet
from pddl.actions import VarValAction
import sas_tasks as fd


def make_sas_task(
    domains: FactSet,
    actions: list[VarValAction],
    init: list[fd.VarValPair],
    goal: list[fd.VarValPair],
) -> fd.SASTask:
    var_index = {var: i for i, var in enumerate(sorted(domains.variables))}
    sas_task = fd.SASTask(
        variables=fd.SASVariables(
            ranges=[len(domains[var]) for var in sorted(domains.variables)],
            axiom_layers=[],
            value_names=[sorted(list(values)) for _, values in domains],
        ),
        mutexes=[
            fd.SASMutexGroup(facts=[(var, i) for var, vals in domains for i in vals])
        ],
        init=fd.SASInit(values=[val for _, val in sorted(init)]),
        goal=fd.SASGoal([(var_index[var], val) for var, val in goal]),
        operators=[
            fd.SASOperator(
                name=a.name,
                prevail=[(var_index[var], val) for var, val in a.prevail],
                pre_post=[(var_index[var], *etc) for var, *etc in a.pre_post],
                cost=a.cost,
            )
            for a in actions
        ],
        axioms=[],
        metric=False,
    )
    return sas_task


def make_vanilla_task(
    domains=FactSet(
        {
            "x": {0, 1, 2},
            "y": {0, 1},
            "z": {0, 1, 2, 3},
        }
    ),
    actions=[
        VarValAction("a1", [("x", 0)], [("x", 1)], 1),
        VarValAction("a2", [("x", 1)], [("y", 1)], 1),
        VarValAction("a3", [("y", 1)], [("z", 1)], 1),
        VarValAction("b1", [("y", 0)], [("x", 2)], 1),
        VarValAction("b2", [("z", 2)], [("z", 3)], 1),
    ],
    init=[
        ("x", 0),
        ("y", 0),
        ("z", 0),
    ],
    goal=[
        ("x", 1),
    ],
):
    info = {
        "domains": domains,
        "actions": actions,
        "init": init,
        "goal": goal,
    }
    return make_sas_task(**info), info


def translate_results(
    relevant_facts: FactSet,
    relevant_actions: list[VarValAction],
    info: dict[FactSet, list[VarValAction], list[fd.VarValPair]],
):
    var_at_index = {i: var for i, var in enumerate(sorted(info["domains"].variables))}
    translated_facts = FactSet(
        {var_at_index[i]: values for i, values in relevant_facts}
    )
    translated_actions = [
        VarValAction(
            a.name,
            [(var_at_index[i], val) for (i, val) in a.precondition],
            [(var_at_index[i], val) for (i, val) in a.effects],
            a.cost,
        )
        for a in relevant_actions
    ]
    return translated_facts, translated_actions


def test_vanilla_values_single():
    sas_task, info = make_vanilla_task()
    relevant_facts, relevant_actions = translate_results(
        *compute_goal_relevance(
            sas_task,
            enable_merging=False,
            enable_causal_links=False,
            variables_only=False,
        ),
        info,
    )

    assert relevant_facts == FactSet({"x": {0, 1}})
    assert sorted([a.name for a in relevant_actions]) == ["a1"]


def test_vanilla_variables_single():
    sas_task, info = make_vanilla_task()
    relevant_facts, relevant_actions = translate_results(
        *compute_goal_relevance(
            sas_task,
            enable_merging=False,
            enable_causal_links=False,
            variables_only=True,
        ),
        info,
    )

    assert relevant_facts == FactSet({"x": {0, 1, 2}, "y": {0, 1}})
    assert sorted([a.name for a in relevant_actions]) == ["a1", "a2", "b1"]


def test_vanilla_values_chain():
    sas_task, info = make_vanilla_task(goal=[("z", 1)])
    relevant_facts, relevant_actions = translate_results(
        *compute_goal_relevance(
            sas_task,
            enable_merging=False,
            enable_causal_links=False,
            variables_only=False,
        ),
        info,
    )

    assert relevant_facts == FactSet({"x": {0, 1}, "y": {1}, "z": {1}})
    assert sorted([a.name for a in relevant_actions]) == ["a1", "a2", "a3"]


def test_vanilla_variables_chain():
    sas_task, info = make_vanilla_task(goal=[("z", 1)])
    relevant_facts, relevant_actions = translate_results(
        *compute_goal_relevance(
            sas_task,
            enable_merging=False,
            enable_causal_links=False,
            variables_only=True,
        ),
        info,
    )

    assert relevant_facts == info["domains"]
    assert sorted([a.name for a in relevant_actions]) == ["a1", "a2", "a3", "b1", "b2"]


# %%
test_vanilla_values_single()
test_vanilla_variables_single()

test_vanilla_values_chain()
test_vanilla_variables_chain()

print("All tests passed.")
