source /xilinx/software/Vivado/2021.2/settings64.sh
echo "vivado -mode batch -source VC707.tcl -tclargs ${1}"
vivado -mode batch -source VC707.tcl -tclargs "$1"
