puts "reading lib.."
foreach libFile [glob "../ASAP7/LIB/*nldm*.lib"] {
    puts "lib: $libFile"
    read_liberty $libFile
}
puts "reading lef.."
read_lef ../ASAP7/techlef/asap7_tech_1x_201209.lef
foreach lef [glob "../ASAP7/LEF/*.lef"] {
    read_lef $lef
}
puts "reading def.."
read_def ../aes_cipher_top/aes_cipher_top.def
read_sdc ../aes_cipher_top/aes_cipher_top.sdc
source ../ASAP7/setRC.tcl
detailed_placement
estimate_parasitics -placement
repair_timing -skip_buffer_removal -skip_pin_swap -skip_gate_cloning

detailed_placement
estimate_parasitics -placement
report_tns
report_wns
report_power

write_def baseline.def
exit
