"""Delete double turns from SAS+ files in /sas_rubiks"""

import os

if __name__ == "__main__":
    for filename in os.listdir("sas_rubiks"):
        new_data = []

        with open(f"sas_rubiks/{filename}") as f:
            data = f.read().strip().split("begin_operator")
            quarter_turn_count = -1
            for section in data:
                action_name = section.split('\n')[1]
                if "2_" not in action_name: # is a quarter turn or intro section:
                    new_data.append(section)
                    quarter_turn_count += 1
            intro_section = new_data[0].split("\n")
            intro_section[-2] = str(quarter_turn_count)
            intro_section = "\n".join(intro_section)
            new_data[0] = intro_section

        new_filename = filename.split(".")
        new_filename[0] = new_filename[0] + "_quarter"
        new_filename = ".".join(new_filename)

        with open(f"sas_rubiks/{new_filename}", "w") as f:
            new_data = "begin_operator".join(new_data)
            f.write(new_data)
