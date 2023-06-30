#Script to change the format of the software results.

file = open("res.txt", "r")
lines = file.read().split("\n")
file.close()

i = 0
while lines[i] != "":
        matrix = ""
        long_line = lines[i].split(":")[-1].split("/")
        matrix += long_line[-1].split(" ")[-1].split(".")[0]
        i += 1
        long_line = lines[i].split(",")
        matrix += " " + long_line[1].split(" ")[-1]
        matrix += " " + long_line[2].split(" ")[-1]
        i += 2
        methods = ["", "", "", ""]
        for j in range(3):
                i += 1
                for k in range(4):
                        long_line = lines[i].split(" ")
                        methods[j+1] += long_line[-1] + " "
                        i += 1
        i += 1
        for j in range(4):
                i += 1
                long_line = lines[i].split(" ")
                methods[j] += long_line[-1] + " "
                i += 1
                long_line = lines[i].split(",")
                methods[j] += long_line[0].split(" ")[-2]
                if len(long_line) != 1:
                        methods[j] += " " +long_line[1].split(" ")[-1]
                i += 1
        string = matrix
        for j in range(4):
                string += " " + methods[j]
        print(string)
        i += 3
        
                
