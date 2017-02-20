File description

==========================================

1. bootld_8051_2.0_fpga.hex

Intel hex file the first verion of bootloader 2.0. - THIS IS THE BOOTLOADER WILL BE USED FOR THE TAPE OUT. 
This bootloader has the CRAM test feature printing out all the addresses and their inforamtion during the test is running.

  * bootloader_8051_2.0_asic.txt
The ascii hex file generated from the file bootld_8051_2.0_fpga.hex above. The content format is 8bit x 8bytes per row


  * bootloader_8051_2.0_asic_8x1.txt
Another version of ascii hex file generated from bootld_8051_2.0_fpga.hex above. But the content format is 8bit x 1byte per row.
This file is used for ASIC tape out.

==========================================

2. bootld_8051_2.0_fpga_07222013.hex (first version)
   bootld_8051_2.1_fpga_08032013.hex (replaced the previous file)

The new bootloader which has the CRAM test feature changed. When testing the CRAM, it only prints the message fail or pass; no more details about the memory cells.

This bootloader doesn't have ascii hex files. This is working bootloader but it is not used for tape out.

==========================================

3. MON51_BANK.hex

Intel hex file of MON51 with banking feature.

 * mon51_bank_asic.txt
   Ascii hex file generated from MON51_BANK.hex. The conetent format is 8bit x 8bytes per row.

 * mon51_bank_asic_8x1.txt
   Another version of ascii hex file generated from MON51_BANK.hex. The content format is 8bit x 1byte per row. This file is used for ASIC tape out.

===========================================



