ACTIVE_ETH_NAME=$(ls /sys/class/net | awk /eth[0-9]*/)

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

## if /opt/greenvity/ghdd/mcps_send1.txt does not exist, generate a new one
if [ ! -f /opt/greenvity/ghdd/mcps_send1.txt ]; then
	mkdir -p /opt/greenvity/ghdd
	touch /opt/greenvity/ghdd/mcps_send1.txt
	
	echo "[wpan_mcps_data_request_t]" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "SrcAddrMode = 3 ; 3 = 64 Bit address, 2 = 16 Bit address, 1 = Reserved, 0 = No Address" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.AddrMode = 3 ; 3 = 64 Bit address, 2 = 16 Bit address, 1 = Reserved, 0 = No Address" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.PANId = 0x4321 ; The 16 bit PAN identifier (0x0000–0xffff)" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte0 = 0x02 ; 64 Bit Address Byte 0" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte1 = 0x00 ; 64 Bit Address Byte 1" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte2 = 0x00 ; 64 Bit Address Byte 2" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte3 = 0x00 ; 64 Bit Address Byte 3" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte4 = 0x00 ; 64 Bit Address Byte 4" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte5 = 0x00 ; 64 Bit Address Byte 5" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte6 = 0x00 ; 64 Bit Address Byte 6" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "DstAddrSpec.Addr.Byte7 = 0x00 ; 64 Bit Address Byte 7" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "TxOptions.bit0 = 1 ;" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "TxOptions.bit1 = 0 ;" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "TxOptions.bit2 = 0 ;" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.SecurityLevel = 0x00 ; The security level purportedly used by the received data frame. (0x00–0x07)" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeyIdMode = 0x00 ; The mode used to identify the key to be used. (0x00–0x03)" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte0 = 0x00 ; Set of 0, 4 or 8 octets | Byte 0" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte1 = 0x00 ; Set of 0, 4 or 8 octets | Byte 1" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte2 = 0x00 ; Set of 0, 4 or 8 octets | Byte 2" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte3 = 0x00 ; Set of 0, 4 or 8 octets | Byte 3" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte4 = 0x00 ; Set of 0, 4 or 8 octets | Byte 4" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte5 = 0x00 ; Set of 0, 4 or 8 octets | Byte 5" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte6 = 0x00 ; Set of 0, 4 or 8 octets | Byte 6" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeySource.Byte7 = 0x00 ; Set of 0, 4 or 8 octets | Byte 7" >> /opt/greenvity/ghdd/mcps_send1.txt
	echo "sec.KeyIndex = 0x00 ; The index of the key purportedly used by the originator of the received frame. (0x01-0xff)" >> /opt/greenvity/ghdd/mcps_send1.txt
fi

sudo insmod ../bin/x86/ghdd.ko
sudo ifconfig ${ACTIVE_ETH_NAME} 10.0.1.2 up