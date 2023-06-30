import os
import codecs
import numpy as np
import matplotlib.pyplot as plt

res_file = open("results.txt", "w")
config = ""
coloring = ""
matrix = ""
write = False
configs = []
matrices = []
cycles = {}

for root, dirs, files in os.walk("."):
    for file in files:
        with codecs.open(os.path.join(root, file), "r", encoding="utf-8", errors="ignore") as infile:
            if os.path.join(root, file).find("SyGS") != -1:
                config = str(os.path.join(root, file).split("/")[1])
                if config not in cycles.keys():
                    cycles[config] = {"graph_coloring": {}, "block_coloring": {}, "dependency_graph": {}}
                res_file.write(config + "\n")
                for line in infile:
                    tokens = line.split()
                    if len(tokens) != 0 and tokens[0] == "Matrix:":
                        matrix = tokens[1]
                        if matrix not in matrices:
                            matrices.append(matrix)
                        write = True
                    elif len(tokens) != 0 and tokens[0] == "Coloring:":
                        coloring = tokens[1]
                    elif len(tokens) != 0 and tokens[0] == "Total" and tokens[1] == "cycles:":
                        if matrix not in cycles[config][coloring]:
                            cycles[config][coloring][matrix] = int(tokens[2])
                    if write:
                        res_file.write(line)
                res_file.write("-" * 50 + "\n\n")
                write = False

'''
sw_res_file = open("../../software_tests/results/final results/results.txt", "r")
for line in sw_res_file:
    if line.find("matrices") != -1:
        matrix = line.split("/")[8]
    else
        tokens = line.split()
        if tokens[0] == "/home"
'''

'''
#                      1138_bus  494_bus   add20     add32
software_cycles_1Th = [7.36E+10, 4.40E+09, 4.14E+09, 1.13E+08]
software_cycles_2Th = [6.81E+10, 6.37E+09, 4.83E+09, 1.10E+08]
software_cycles_4Th = [5.91E+10, 5.99E+09, 4.20E+09, 8.27E+07]

matrices.remove("bcsstk14")
matrices.remove("bcsstk15")
matrices.remove("bcsstk27")
matrices.remove("fs_760_2")
matrices = sorted(matrices)
print(str(matrices))
print(str(cycles))

graph_cycles_1PE = []
block_cycles_1PE = []
dep_cycles_1PE = []
for matrix in matrices:
    if matrix != "add32":
        graph_cycles_1PE.append(cycles["SyGS_1PE_4VarBanks_64bit"]["graph_coloring"][matrix])
        block_cycles_1PE.append(cycles["SyGS_1PE_4VarBanks_64bit"]["block_coloring"][matrix])
        dep_cycles_1PE.append(cycles["SyGS_1PE_4VarBanks_64bit"]["dependency_graph"][matrix])

graph_cycles_1PE.append(71469843)  # add32

graph_cycles_2PE = []
block_cycles_2PE = []
dep_cycles_2PE = []
for matrix in matrices:
    graph_cycles_2PE.append(cycles["SyGS_2PE_4VarBanks_64bit"]["graph_coloring"][matrix])
    block_cycles_2PE.append(cycles["SyGS_2PE_4VarBanks_64bit"]["block_coloring"][matrix])
    dep_cycles_2PE.append(cycles["SyGS_2PE_4VarBanks_64bit"]["dependency_graph"][matrix])

graph_cycles_4PE = []
block_cycles_4PE = []
dep_cycles_4PE = []
for matrix in matrices:
    graph_cycles_4PE.append(cycles["SyGS_4PE_4VarBanks_64bit"]["graph_coloring"][matrix])
    block_cycles_4PE.append(cycles["SyGS_4PE_4VarBanks_64bit"]["block_coloring"][matrix])
    dep_cycles_4PE.append(cycles["SyGS_4PE_4VarBanks_64bit"]["dependency_graph"][matrix])

graph_speedups_2Th = list(map(lambda tup: tup[1] / tup[0], zip(software_cycles_2Th, software_cycles_1Th)))
graph_speedups_4Th = list(map(lambda tup: tup[1] / tup[0], zip(software_cycles_4Th, software_cycles_1Th)))
graph_speedups_1PE = list(map(lambda tup: tup[1] / tup[0], zip(graph_cycles_1PE, software_cycles_1Th)))
graph_speedups_2PE = list(map(lambda tup: tup[1] / tup[0], zip(graph_cycles_2PE, software_cycles_1Th)))
graph_speedups_4PE = list(map(lambda tup: tup[1] / tup[0], zip(graph_cycles_4PE, software_cycles_1Th)))

print(graph_speedups_1PE)
print(graph_speedups_2PE)
print(graph_speedups_4PE)
print(graph_speedups_4Th)

# block_speedups = list(map(lambda tup: tup[1] / tup[0], zip(block_cycles, software_cycles_1Th)))
# dep_speedups = list(map(lambda tup: tup[1] / tup[0], zip(dep_cycles, software_cycles_1Th)))

fig = plt.gcf()
fig.set_size_inches(9, 7)

X = matrices
X_axis = np.arange(len(X) * 1.6, step=1.6)

plt.bar(x=X_axis - 0.5, height=1, width=0.2, label="1 Thread sequential", color=(0/255, 153/255, 255/255))
plt.bar(x=X_axis - 0.3, height=graph_speedups_2Th, width=0.2, label="2 Threads graph coloring", color=(51/255, 102/255, 204/255))
plt.bar(x=X_axis - 0.1, height=graph_speedups_4Th, width=0.2, label="4 Threads graph coloring", color=(0/255, 51/255, 153/255))
plt.bar(x=X_axis + 0.1, height=graph_speedups_1PE, width=0.2, label="1 PE graph coloring", color=(0/255, 240/255, 0/255))
plt.bar(x=X_axis + 0.3, height=graph_speedups_2PE, width=0.2, label="2 PEs graph coloring", color=(0/255, 204/255, 0/255))
plt.bar(x=X_axis + 0.5, height=graph_speedups_4PE, width=0.2, label="4 PEs graph coloring", color=(0/255, 153/255, 0/255))

plt.xticks(X_axis, X)
plt.xlabel("Matrix")
plt.ylabel("Clock cycles speed-up")
plt.legend()
plt.show()
'''
