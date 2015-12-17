rmmod gv701x_spi_intf
insmod /opt/greenvity/ghdd/gv701x_spi_intf.ko
insmod /opt/greenvity/ghdd/gv701x_spi_net.ko
sleep 1
ifconfig gvspi hw ether 10:22:33:44:55:66
ifconfig eth0  up
sleep 1
ifconfig gvspi 10.0.1.1 up
sleep 1

## if ghdd_config.txt does not exist, generate a new one
if [ ! -f /opt/greenvity/ghdd/ghdd_config.txt ]; then
	mkdir -p /opt/greenvity/ghdd
	rm -f /opt/greenvity/ghdd/ghdd_config.txt
	rm -f ghdd_config.txt
	touch ghdd_config.txt
	echo "MAC_Address = AA:11:22:33:44:68" >> ghdd_config.txt
	echo "linemode = DC" >> ghdd_config.txt
	echo "DC_freq = 50" >> ghdd_config.txt
	echo "txpowermode = 255" >> ghdd_config.txt
	echo "ER_mode = disabled" >> ghdd_config.txt
	echo "gv701x_interface = spi" >> ghdd_config.txt
	echo "attach_to = gvspi" >> ghdd_config.txt
	echo "passwd = greenvity" >> ghdd_config.txt
	mv ghdd_config.txt /opt/greenvity/ghdd/ghdd_config.txt
fi

"/root/turn_on_wifi.sh"
sleep 1
#insmod /opt/greenvity/ghdd/ghdd.ko
"/opt/greenvity/prodTool/gv_server.out"
