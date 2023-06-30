#Script to change the format of the software results.

file = open("largest.txt", "r")
lines = file.read().split("\n")
file.close()

i = 0
while lines[i] != "":
	i += 1
i += 2

while lines[i] != "":
        line = lines[i].split("/")
        info = line[-1].split(" ")
        info[0] = info[0][:-5]
        long_line = ""
        for elem in info:
                long_line += elem + " "
        long_line += lines[i+1][:-1] + " " + lines[i+2][:-1] + " " + lines[i+3][:-1] + " " + lines[i+4][:-1]
        i += 6
        print(long_line)
i += 3
print("\n\n\n")

while lines[i] != "":
        line = lines[i].split("/")
        info = line[-1].split(" ")
        info[0] = info[0][:-5]
        long_line = ""
        for elem in info:
                long_line += elem + " "
        long_line += lines[i+1][:-1] + " " + lines[i+2][:-1] + " " + lines[i+3][:-1] + " " + lines[i+4][:-1]
        i += 6
        print(long_line)
i += 3
print("\n\n\n")

while i < len(lines)-1:
        line = lines[i].split("/")
        info = line[-1].split(" ")
        info[0] = info[0][:-5]
        long_line = ""
        for elem in info:
                long_line += elem + " "
        long_line += lines[i+1][:-1] + " " + lines[i+2][:-1] + " " + lines[i+3][:-1] + " " + lines[i+4][:-1]
        i += 6
        print(long_line)



