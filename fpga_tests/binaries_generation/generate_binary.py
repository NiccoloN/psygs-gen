# Generate binaries for all datasets for the given configurations

import sys
import os

matrix_path = sys.argv[1]
matrix_name = os.path.basename(matrix_path)
config = sys.argv[2]
coloring_name = sys.argv[3]
coloring = "0"
if coloring_name == "graph_coloring":
    coloring = "1"
elif coloring_name == "block_coloring":
    coloring = "2"   # can concat block size
elif coloring_name == "dependency_graph":
    coloring = "3"
else:
    print(
        "\u001b[31mError occurred while generating binary for " + matrix_path + " and " + config + "with" + coloring_name
        + ": invalid coloring" + "\u001b[0m\n")
    exit(1)

print("Using " + sys.argv[3] + " as " + coloring)

params = []

# read configuration parameters from file
with open("../sygs_template/configurations.txt", 'r') as infile:
    for line in infile:
        tokens = line.split()
        if len(tokens) == 0 or config != tokens[0]:
            continue
        for param in tokens[2:]:
            params.append(param)

n_acc = str(params[0])
acc_bank_size = str(params[1])
n_var_banks = str(params[2])
var_bank_size = str(params[3])

print("Generating binaries for config " + config + "\n")

# set accelerator constants
os.system("python3 generate_template_constants.py " +
          n_acc + " " + acc_bank_size + " " + n_var_banks + " " + var_bank_size)

# preprocessing: coloring + generate data_allocation.h and data_allocation_global.h
os.chdir("../preprocessing")
if "-first" in sys.argv:
    os.system("make clean")
    os.system("make")
    print()
if not os.path.isdir("../../resources/vectors"):
    os.system("mkdir ../../resources/vectors")
print("Starting preprocessing")
os.system("./preprocess " + matrix_path + " " + n_var_banks + " " + n_acc + " no-rand " +
          "../../resources/vectors/" + matrix_name[:matrix_name.rfind(".")] + ".vec" + " " + coloring)
print("Preprocessing done!\n")

# TODO set RISCV env path
riscv_env_path = "../../../env.sh"

# compile hostcode binary
os.chdir("../hostcode")
os.system("source " + riscv_env_path)
os.system("make clean")
os.system("make")
if os.path.exists("build/compiled.bin"):
    bin_str = config + "_" + matrix_name + "_" + coloring_name + ".riscv"
    symt_str = config + "_" + matrix_name + "_" + coloring_name + ".symt"
    print("\nCompilation successful, binary saved as " + bin_str + "\n")
    os.system("cp ./build/compiled.bin ./../binaries_generation/binaries/" + bin_str)
    os.system("cp ./build/compiled.symt ./../binaries_generation/binaries/" + symt_str)
else:
    print(
        "\u001b[31mError occurred while generating binary for " + matrix_path + " and " + config + "with" + coloring_name
        + ": compiled.bin does not exists" + "\u001b[0m\n")
    exit(1)
os.chdir("../binaries_generation")

# generate data binary

# names of the global variables declared in the hostcode to contain matrix data (data_allocation_global.h)
data = ['var_data', 'var_off', 'acc_data', 'acc_off']
addr_dict = {}
with open("binaries/" + symt_str, 'r') as infile:
    for line in infile:
        tokens = line.split()
        if len(tokens) == 0:
            continue
        name = tokens[len(tokens) - 1]
        if name in data:
            addr_dict[name] = int(tokens[0], 16)

addresses = ""
for name in data:
    addresses += str(addr_dict[name]) + " "

os.system("make clean")
os.system("make")
os.system("./generate_binary_data " + config + "_" + matrix_name + "_" + coloring_name + " " + addresses)

print("binary generation done\n")
