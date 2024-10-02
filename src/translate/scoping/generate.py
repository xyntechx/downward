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
            axiom_layers=[-1 for _ in domains.variables], # No axioms == -1
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
            0: {0, 1, 2},
            1: {0, 1},
            2: {0, 1, 2, 3},
        }
    ),
    actions=[
        VarValAction(" a1 ", [(0, 0)], [(0, 1)], 1),
        VarValAction(" a2 ", [(0, 1)], [(1, 1)], 1),
        VarValAction(" a3 ", [(1, 1)], [(2, 1)], 1),
        VarValAction(" b1 ", [(1, 0)], [(0, 2)], 1),
        VarValAction(" b2 ", [(2, 2)], [(2, 3)], 1),
    ],
    init=[
        (0, 0),
        (1, 0),
        (2, 0),
    ],
    goal=[
        (0, 1),
    ],
):
    info = {
        "domains": domains,
        "actions": actions,
        "init": init,
        "goal": goal,
    }
    return make_sas_task(**info), info


def make_cherry_task():
    """
    Each operator has two actions
    """
    return make_vanilla_task(
        domains=FactSet(
            {
                0: {0, 1, 2},
                1: {0, 1},
                2: {0, 1, 2, 3},
            } 
        ), 
        actions=[ 
            VarValAction(" a1 ", [(0, 0), (1, 0)], [(2, 0), (0, 2)], 1),
            VarValAction(" a2 ", [(0, 1), (0, 1)], [(1, 1), (2, 0)], 1),
            VarValAction(" a3 ", [(2, 1), (1, 0)], [(2, 1), (1, 1)], 1),
            VarValAction(" a4 ", [(1, 1), (1, 1)], [(0, 1), (2, 3)], 1),
            VarValAction(" a5 ", [(2, 0), (0, 2)], [(2, 3), (0, 1)], 1)
        ],
        init=[
            (0, 0),
            (1, 0),
            (2, 0),
        ],
        goal=[
            (0, 1),
        ],
    )


if __name__ == "__main__":
    tasks = {
        "vanilla": make_vanilla_task(), # solution is a1
        "cherry": make_cherry_task(), # solution is a1-a5
    }
    
    for task_name in tasks:
        sas_task, info = tasks[task_name]
        filepath = f"sas_custom/{task_name}.sas"
        with open(filepath, "w") as f:
            sas_task.output(f)
        print(f"SAS+ saved at {filepath}")
