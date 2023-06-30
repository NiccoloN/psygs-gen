#Script that verifies if the provided matrix is a valid lower triangular one (values in the diagonal allowed).

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("path")
args = parser.parse_args()
filename = args.path

file = open(filename, "r")
lines = file.read().split("\n")
file.close()

#obtaining the size and number of nonzeros of the matrix
line = 0
while True:
    if lines[line][0] != "%":
        size = int(lines[line].split(" ")[0])
        break
    line += 1
info_line = line
num_nz = int(lines[line].split(" ")[-1])

ok = True
while lines[line] != "" and ok:
    split_line = lines[line].split(" ")
    if int(split_line[0]) < int(split_line[1]):
        ok = False
    line += 1
print(ok)
    

