
import sys

# Generate constants for a sygs template configuration


def generate_constants(n_acc, acc_bank_size, n_var_banks, var_bank_size):
    header_guard = "#ifndef TEMPLATE_CONSTANTS_H\n#define TEMPLATE_CONSTANTS_H\n"
    header_end = "#endif // TEMPLATE_CONSTANTS_H\n"

    print("Set NUM_ACC to " + str(n_acc))
    print("Set ACC_BANK_SIZE to " + str(acc_bank_size))
    print("Set NUM_VAR_BANKS to " + str(n_var_banks))
    print("Set VAR_BANK_SIZE to " + str(var_bank_size) + "\n")

    define_str = "#define "
    opcodes = "#define SYGS_TMPL_OP_WREQ 11\n" \
              "#define SYGS_TMPL_OP_WMEM 43\n" \
              "#define SYGS_TMPL_OP_RMEM 91\n" \
              "#define SYGS_TMPL_OP_EXEC 123\n"

    with open('../hostcode/generated/template_constants.h', 'w') as infile:
        infile.write(header_guard)
        infile.write(define_str + "NUM_ACC " + str(n_acc) + "\n")
        infile.write(define_str + "ACC_BANK_SIZE " + str(acc_bank_size) + "\n")
        infile.write(define_str + "NUM_VAR_BANKS " + str(n_var_banks) + "\n")
        infile.write(define_str + "VAR_BANK_SIZE " + str(var_bank_size) + "\n")
        infile.write(opcodes)
        infile.write(header_end)


n_acc = int(sys.argv[1])
acc_bank_size = int(sys.argv[2])
n_var_banks = int(sys.argv[3])
var_bank_size = int(sys.argv[4])
generate_constants(n_acc, acc_bank_size, n_var_banks, var_bank_size)
