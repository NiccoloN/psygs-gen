import os
import sys

#matrixArray = ["smol", "494_bus", "1138_bus", "add20", "add32", "bcsstk14", "bcsstk15", "bcsstk27", "fs_760_2"]
matrixArray = ["494_bus", "1138_bus", "add20", "add32"]
colorings = ["graph_coloring",
             "block_coloring",
             "dependency_graph"]

# read from file
configArray = []
bitstreamArray = []

# enter the directory containing this script
os.chdir(sys.path[0])

compile_binaries = True
if "-no-compile" in sys.argv:
    compile_binaries = False

deploy = True
if "-no-deploy" in sys.argv:
    deploy = False

show_res = "-save"
if "-show-result" in sys.argv:
    show_res = "-no-save"

debug = "-no-debug"
if "-debug" in sys.argv:
    debug = "-debug"

firstCycle = "-first"

# read configurations from file
with open("sygs_template/configurations.txt", 'r') as infile:
    for line in infile:
        tokens = line.split()
        if len(tokens) == 0:
            continue
        configArray.append(tokens[0])
        bitstreamArray.append(tokens[1])

for config, bitstream in zip(configArray, bitstreamArray):

    if not os.path.isfile("sygs_template/bitstreams/" + bitstream):
        print("bitstream " + bitstream + " not found")
        exit(1)

    for matrix in matrixArray:

        for coloringType in colorings:

            if not os.path.isfile("../resources/matrices/" + matrix + ".mtx"):
                print("matrix " + matrix + " not found")
                exit(1)

            print("\nUsing bitstream: " + bitstream)

            if compile_binaries:
                print("Compiling binary...")
                os.system("bash binaries_generation/generate_binary.sh " + matrix + " " + config + " " + coloringType + " " + firstCycle)
                firstCycle = "-no-first"

            if deploy:
                print("Programming device...")
                os.system("bash ./program_device.sh " + bitstream)
                print("Loading binary...")
                os.system("bash ./send_binary.sh binaries/" + config + "_" + matrix + ".mtx_" + coloringType + ".exec.bin")
                print("Testing...")
                os.system("bash -c \"./exec_test.sh " + matrix + " " + config + " " + bitstream + " " + coloringType +
                          " " + show_res + " " + debug + "\"")
                print("Test complete\n")
                #os.chdir("results")
                #os.system("python3 compact_results.py")
                #os.chdir("..")
                #print("Results updated\n")

print("All done\n")
