#Script to change the format of the software results and create graphs.


import matplotlib.pyplot as plt
import numpy as np

file = open("results.txt", "r")
lines = file.read().split("\n")
file.close()

matrices = []
standard = []
graph_coloring = []
block_coloring = []
dependency_graph = []
names = ["graph coloring", "block coloring", "dependency graph"]


i = 3
while lines[i] != "":
        matrix = ""
        long_line = lines[i].split(":")[-1].split("/")
        matrix += long_line[-1].split(" ")[-1].split(".")[0]
        matrices.append(matrix)
        i += 1
        std = []
        long_line = lines[i].split(" ")
        std.append(float(long_line[0]))
        std.append(float(long_line[1]))
        standard.append(std)
        i += 2
        for item in [graph_coloring, block_coloring, dependency_graph]:
                temp2 = []
                for j in range(6):
                        temp = []
                        long_line = lines[i].split(" ")
                        for num in long_line:
                                if num != "":
                                        temp.append(float(num))
                        i += 1
                        temp2.append(temp)
                item.append(temp2)
                i += 2
        i += 1

for i in range(len(matrices)):
        print(matrices[i])
        print("standard ")
        res = "1 "
        for line in standard[i]:
                if(float(int(line)) == line):
                        res += str(int(line)) + " "
                else:
                        res += str(line) + " "
        print(res)
        print("")
        index = 0
        for item in [graph_coloring, block_coloring, dependency_graph]:
                print(names[index])
                for j in range(4):
                        res = ""
                        for k in range(len(item[i])):
                                if(float(int(item[i][k][j])) == item[i][k][j]):
                                        res += str(int(item[i][k][j])) + " "
                                else:
                                        res += str(item[i][k][j]) + " "
                        print(res)
                print("")
                index += 1
        print("")


index = 0
for item in [graph_coloring, block_coloring, dependency_graph]:
        for kind in ["", "adjusted"]:
                if kind == "":
                        #compute line of averages
                        x_m = item[0][0]
                        y_m = [0.0]*len(x_m)
                        for matrix in item:
                                for i in range(len(matrix[3])):
                                        y_m[i] += matrix[3][i]
                        for i in range(len(x_m)):
                                y_m[i] /= len(item)
                        x_m = [0] + x_m
                        y_m = [0.0] + y_m
                        
                        #compute scatterplot values
                        x = item[0][0]*len(item)
                        y = []
                        for matrix in item:
                                y += matrix[3]
                else:
                        #compute line of averages
                        x_m = item[0][0]
                        y_m = [0.0]*len(x_m)
                        for matrix in item:
                                for i in range(len(matrix[3])):
                                        y_m[i] += matrix[3][i]/matrix[4][i]*matrix[0][i]
                        for i in range(len(x_m)):
                                y_m[i] /= len(item)
                        x_m = [0] + x_m
                        y_m = [0.0] + y_m
                        
                        #compute scatterplot values
                        x = item[0][0]*len(item)
                        y = []
                        for matrix in item:
                                for i in range(len(matrix[3])):
                                        y.append(matrix[3][i]/matrix[4][i]*matrix[0][i])
                        
                # plot
                sizes = [64]*len(x)
                colors = []
                temp_colors = []
                for i in range(len(x)//4):
                        temp_colors.append(int(100/(len(x)/4)*i))
                for color in temp_colors:
                        for i in range(4):
                                colors.append(color)
                fig, ax = plt.subplots()
                ax.scatter(x, y, s=sizes, c=colors, vmin=0, vmax=100)
                if kind == "":
                        ax.set(xlim=(0, 8), xticks=np.arange(1, 8), ylim=(0, 4), yticks=np.arange(1, 4))
                else:
                        limit = max(1.1*max(y_m),1.1*max(y))
                        ax.set(xlim=(0, 8), xticks=np.arange(1, 8), ylim=(0, limit), yticks=np.arange(1, limit))
                plt.plot(x_m, y_m)
                plt.xlabel("Number of threads")
                plt.ylabel("Speedup")
                if kind == "":
                        plt.title("Real "+kind+names[index]+" speedup")
                        plt.savefig(names[index]+kind+".png")
                else:
                        plt.title("Real "+kind+" "+names[index]+" speedup")
                        plt.savefig(names[index]+" "+kind+".png")
        
        index += 1
                
