import subprocess
import os

pddls = []

for domain in os.listdir("pddl"):
    for file in os.listdir(f"pddl/{domain}"):
        if "problem" in file:
            pddls.append(f"pddl/{domain}/{file}")

for path in pddls:
    _, domain, problem = path.split("/")
    problem_id, _ = problem.split(".")
    sas_file = f"sas_cam/{domain}/{problem_id}.sas"
    subprocess.call(["./fast-downward.py", "--sas-file", f"{sas_file}", "--translate", f"{path}"])
