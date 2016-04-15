Greenvity Virtual Ethernet Dual Driver (GVEDD)
==============================================

This repository contains supporting source code for the Greenvity Virtual Ethernet Dual Driver.
To compile for ARM, use:
- **Cross-compiler** arm-2011.09-70-arm-none-linux-gnueabi.bin
- **Host** Ubuntu 10.10 x86 
- **Build against image generated from** L2.6.31_10.05.02_ER_source.tar.gz
- **Make instructions for x86:** `make TARGET=x86`
- **Make instructions for i.MX233:** `make TARGET=imx233`

It contains the following folders:

gvedd - GVEDD Kernel Module (.ko)
---------------------------------

- Contains the source code for the kernel module (will build: gvedd.ko)
- This kernel module registers two Ethernet network devices : **hpgp** and **lrwpan**
- The kernel modules also seperates out the management frames and makes it available over netlink to a userspace program
- The module can send data to the Hybrii chip using either of the two interfaces: Ethernet or SPI. This option is configurable.
	- For Ethernet it uses whatever driver is preloaded in the system and hooks onto it. If using the Asix USB to Ethernet chip, load the asix driver before loading GVEDD
	- For SPI (available only for i.MX233), it uses the virtual device **spi** created by our SPI driver for i.MX233
- Upper layers (Linux TCP/IP Stack or Greenvity's TCP/IPv6 Stack) must use GVEDD to communicate with the hybrii chip.
- Compiles into a Loadable Kernel Module (.ko)
- Kernel module insertion examples:
	- For Ethernet `sudo insmod gvedd.ko attach_to=eth0 gv710x_interface=ether`
	- For SPI `sudo insmod gvedd.ko attach_to=gvspi gv710x_interface=spi`
- In case of SPI you will have to load **gvspi_net** and **gvspi_proto** before loading **gvedd**.     
This is because **gvedd** uses **gvspi_net** which in turn uses **gvspi_proto**.

cli - GVEDD Command Line Interface (.out)
-----------------------------------------

- Contains userspace program to communicate with the kernel module over netlink to configure the hpgp driver and send/receive management frames.
- Compiles into a Userspace Application (.out)
- Note that lower level drivers (for ethernet : gvedd (and asix driver in case of imx233), for SPI - gvedd and gvspi) must be installed before running cli application.
- Executing the CLI without any arguments will list all commands and options: `sudo ./gvedd_cli_x86.out`   
- Here is a list of all the options:
`sudo ./gvedd_cli_x86.out hpgp secmode <security mode> : Set Security Mode`   
`sudo ./gvedd_cli_x86.out hpgp secmode                 : Get Security Mode`   
`sudo ./gvedd_cli_x86.out hpgp netid <password> <security level>     : Set Default Net Id`   
`sudo ./gvedd_cli_x86.out hpgp restartsta              : Restart STA`     
`sudo ./gvedd_cli_x86.out hpgp network <net option>    : Set Network`   
`sudo ./gvedd_cli_x86.out hpgp netexit                 : Net Exit`    
`sudo ./gvedd_cli_x86.out hpgp appointcco <MAC address>: Appoint CCO`   
`sudo ./gvedd_cli_x86.out hpgp authsta <password>      : Authorize STA`     
`sudo ./gvedd_cli_x86.out hpgp datapath <1/0>`    
`sudo ./gvedd_cli_x86.out hpgp sniffer <1/0>`    
`sudo ./gvedd_cli_x86.out hpgp bridge <1/0>`    
`sudo ./gvedd_cli_x86.out hpgp devmode`     
`sudo ./gvedd_cli_x86.out hpgp hwspec`     
`sudo ./gvedd_cli_x86.out hpgp devstats`     
`sudo ./gvedd_cli_x86.out hpgp peerinfo`     
	-         Security Mode:       
		          0: Secure.       
		          1: Simple-Connect.       
		          2: SC-Add.       
		          3: SC-Join.       
	-         Security Level:       
		          0: HS    
				  1: SC       
	-         Network option:       
		          0: Start the netowrk as a CCo.       
		          1: Join the network as a STA.       
	-         MAC address format: AA:22:CC:44:FE:34       
	-         Password: 8 to 64 characters long       



gvspi/gvspi_net - SPI Virtual Ethernet Driver (.ko)
---------------------------------------------

- Registers an Ethernet Network Device on Linux which transmits/receives Ethernet frames to/from the Hybrii chip over the SPI interface.
- Used by the kernel_module when configured to operate in SPI mode.
- Compiles into a Loadable Kernel Module (.ko) `gv710x_spi_net.ko`
- **gvspi_proto** must be loaded before **gvspi_net** can be loaded.

gvspi/gvspi_proto - Low-level SPI peripheral driver for i.MX233 (.ko)
---------------------------------------------------------------

- Manipulates the internal registers of i.MX233 to send and receives bytes of data over the Serial Peripheral Interface (SPI).
- Compiles into a Loadable Kernel Module (.ko) `gv710x_spi_intf.ko`

scripts - Linux shell scripts to build and install the GVEDD kernel modules on the target system(.sh)
-----------------------------------------------------------------------------------------------------
- **gv_make_eth_imx233.sh**
- **gv_make_eth_x86.sh**
- **gv_make_spi_imx233.sh**
- **gv_run_eth_x86.sh**
- **gv_run_spi_imx233.sh**
