## if ghdd_config.txt does not exist, generate a new one
## if /opt/greenvity/ghdd/ghdd_config.txt does not exist, generate a new one
if [ ! -f /opt/greenvity/ghdd/ghdd_config.txt ]; then
	mkdir -p /opt/greenvity/ghdd
	touch ghdd_config.txt
	echo "macAddress = AA:11:22:33:44:56" >> ghdd_config.txt
	echo "lineMode = DC" >> ghdd_config.txt
	echo "dcFreq = 50" >> ghdd_config.txt
	echo "txPowerMode = 255" >> ghdd_config.txt
	echo "erMode = disabled" >> ghdd_config.txt
	echo "busInterface = ether" >> ghdd_config.txt
	echo "deviceMode = AUTO" >> ghdd_config.txt
	echo "netInterface =  ${ACTIVE_ETH_NAME}" >> ghdd_config.txt
	echo "passwd = greenvity" >> ghdd_config.txt
	mv ghdd_config.txt /opt/greenvity/ghdd/ghdd_config.txt
fi


insmod /opt/greenvity/ghdd/ghdd.ko
ifconfig eth0 10.0.1.2 up