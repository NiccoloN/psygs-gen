#if {$argc != 2} {
#  puts "Expected: <config>"
#  exit
#}


set my_config [lindex $argv 0]
puts ${my_config}
open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target
current_hw_device [get_hw_devices xc7vx485t_0]
refresh_hw_device -update_hw_probes false [lindex [get_hw_devices xc7vx485t_0] 0]
set_property PROBES.FILE {} [get_hw_devices xc7vx485t_0]
set_property FULL_PROBES.FILE {} [get_hw_devices xc7vx485t_0]
set_property PROGRAM.FILE sygs_template/bitstreams/${my_config} [get_hw_devices xc7vx485t_0]
program_hw_devices [get_hw_devices xc7vx485t_0]
refresh_hw_device [lindex [get_hw_devices xc7vx485t_0] 0]
