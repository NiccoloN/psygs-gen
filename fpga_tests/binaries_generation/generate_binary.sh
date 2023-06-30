cd "${BASH_SOURCE%/*}" || exit
if [ ! -d "binaries" ]
then mkdir "binaries"
fi
python3 generate_binary.py ../../resources/matrices/"$1".mtx "$2" "$3" "$4"
if test -f "binaries/${2}_${1}_${3}.exec.bin";
then rm binaries/"$2"_"$1"_"$3".exec.bin
fi
ls -la binaries/"$2"_"$1".mtx_"$3".riscv
ls -la binaries/"$2"_"$1".mtx_"$3"-data.bin
cat binaries/"$2"_"$1".mtx_"$3".riscv binaries/"$2"_"$1".mtx_"$3"-data.bin > binaries/"$2"_"$1".mtx_"$3".exec.bin
