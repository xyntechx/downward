from argparse import ArgumentParser


parser = ArgumentParser()
parser.add_argument("-s", "--sas-file", dest="sas_src")
parser.add_argument("-m", "--macro-file", dest="macro_src")
args = parser.parse_args()

macros = []

with open(args.macro_src) as macro_file:
    content = macro_file.read().strip().split("\n")
    macro_idx = -1

    for i in range(len(content)):
        line = content[i]
        if line == "begin_macro":
            macros.append({})
            macro_idx += 1
        elif line == "begin_macro_name":
            macros[macro_idx]["name"] = content[i+1]
        elif line == "begin_macro_prevails":
            macros[macro_idx]["prevails"] = []
            j = i + 1
            while "begin" not in content[j]:
                var, val = content[j].split("=")
                macros[macro_idx]["prevails"].append((int(var), int(val)))
                j += 1
        elif line == "begin_macro_effects":
            macros[macro_idx]["effects"] = []
            eff_idx = 0
        elif line == "begin_eff_precondition":
            macros[macro_idx]["effects"].append({})
            if "begin" not in content[i+1]:
                var, val = content[i+1].split("=")
                macros[macro_idx]["effects"][eff_idx]["precondition"] = (int(var), int(val))
            else:
                macros[macro_idx]["effects"][eff_idx]["precondition"] = ()
        elif line == "begin_eff_conditions":
            macros[macro_idx]["effects"][eff_idx]["eff_conditions"] = []
            j = i + 1
            while "begin" not in content[j]:
                var, val = content[j].split("=")
                macros[macro_idx]["effects"][eff_idx]["eff_conditions"].append((int(var), int(val)))
                j += 1
        elif line == "begin_eff_fact":
            var, val = content[i+1].split("=")
            macros[macro_idx]["effects"][eff_idx]["post"] = (int(var), int(val))
            eff_idx += 1
        elif line == "begin_macro_cost":
            macros[macro_idx]["cost"] = int(content[i+1])
        elif line == "begin_macro_isaxiom":
            macros[macro_idx]["is_an_axiom"] = bool(int(content[i+1]))


new_sas_content = []


with open(args.sas_src) as sas_file:
    sas_content = sas_file.read().strip().split("\n")
    has_inserted_macro = False

    for i in range(len(sas_content)):
        if i > 0 and sas_content[i-1] == "end_goal":
            new_sas_content.append(str(int(sas_content[i]) + len(macros)))
        elif sas_content[i] == "begin_operator" and not has_inserted_macro:
            for macro in macros:
                new_sas_content.append("begin_operator")
                new_sas_content.append(macro["name"])
                new_sas_content.append(str(len(macro["prevails"])))
                for prevail in macro["prevails"]:
                    new_sas_content.append(f"{prevail[0]} {prevail[1]}")
                new_sas_content.append(str(len(macro["effects"])))
                for eff in macro["effects"]:
                    line = ""
                    line += f"{len(eff['eff_conditions'])} "
                    for cond in eff["eff_conditions"]:
                        line += f"{cond[0]} {cond[1]} "
                    if eff['precondition']:
                        line += f"{eff['precondition'][0]} {eff['precondition'][1]} {eff['post'][1]}"
                    else:
                        line += f"{eff['post'][0]} -1 {eff['post'][1]}"
                    new_sas_content.append(line)
                new_sas_content.append(str(macro["cost"]))
                new_sas_content.append("end_operator")
            new_sas_content.append("begin_operator")
            has_inserted_macro = True
        else:
            new_sas_content.append(sas_content[i])


with open(f"{'/'.join(args.sas_src.split('/')[:-1])}/with_macros_{args.sas_src.split('/')[-1][-7:-4]}.sas", "w") as sas_file:
    sas_file.write('\n'.join(new_sas_content))
