#script to make the provided matrix strictly diagonally dominant (not used for the final results).

import random
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("path")
args = parser.parse_args()
filename = args.path

file = open(filename, "r")
lines = file.read().split("\n")
file.close()

#obtaining the size of the matrix
line = 0
while True:
    if lines[line][0] != "%":
        size = int(lines[line].split(" ")[0])
        break
    line += 1
info_line = line
old_num_nz = int(lines[line].split(" ")[-1])

#initializing a list that stores whether the ith row has a nonzero diagonal value
#storing the sum of the absolute values of the elements in each row
diagonal_values = []
min_value_by_line = []
for i in range(size):
    diagonal_values.append(False)
    min_value_by_line.append(0)
    
#veryfining what rows have a nonzero diagonal value
#calculating the sum of the absolute values of the elements of each row
max_value = 0
line += 1
while line < len(lines):
    if lines[line] == "":
        break
    if lines[line].split(" ")[0] == lines[line].split(" ")[1]:
        diagonal_values[int(lines[line].split(" ")[0])-1] = True
    else:
        min_value_by_line[int(lines[line].split(" ")[0])-1] += abs(float(lines[line].split(" ")[-1]))
    if abs(float(lines[line].split(" ")[-1])) > max_value:
        max_value = abs(float(lines[line].split(" ")[-1]))
    line += 1

#making the matrix strictly diagonally dominant
#by adding += a value between 1 and 2 times the sum of the absolute values of the elements of the row + 0.001*max_value
file = open(filename, "a")
changed = 0
for i in range(size):
    if not diagonal_values[i]:
        amount = (1+random.random())*min_value_by_line[i]+0.001*max_value
        if random.randint(0, 1) == 1:
            amount *= -1
        file.write(str(i+1)+" "+str(i+1)+" "+str(amount)+"\n")
        changed += 1
file.close()

#correcting the number of nonzeros in the matrix
file = open(filename, "r")
data = file.readlines()
file.close()
data[info_line] = str(size)+" "+str(size)+" "+str(old_num_nz+changed)+"\n"
file = open(filename, "w")
file.writelines(data)
file.close()

#fixing already existing diagonal values to have a strictly diagonally dominant matrix
line = info_line+1
while line < len(data):
    if data[line] == "":
        break
    if data[line].split(" ")[0] == data[line].split(" ")[1]:
        if abs(float(data[line].split(" ")[-1])) <= min_value_by_line[int(data[line].split(" ")[0])-1]:
            amount = (1+random.random())*min_value_by_line[int(data[line].split(" ")[0])-1]+0.001*max_value
            if random.randint(0, 1) == 1:
                amount *= -1
            data[line] = data[line].split(" ")[0] + " " + data[line].split(" ")[1] + " " +  str(amount) + "\n"
    line += 1
file = open(filename, "w")
file.writelines(data)
file.close()

