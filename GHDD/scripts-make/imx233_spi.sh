cd ../gvspi/gvspi_net
make TARGET=imx233
cd -

cd ../ghdd
make TARGET=imx233
cd -

cd ../cli
make TARGET=imx233
cd -

cd ../event-logger
make TARGET=imx233
cd -

rm -rf ../bin/imx233_spi/gv701x_spi_net.ko
rm -rf ../bin/imx233_spi/ghdd.ko
rm -rf ../bin/imx233_spi/ghdd_cli_imx233.out
rm -rf ../bin/imx233_spi/ghdd_event_imx233.out

mkdir -p ../bin/imx233_spi

mv ../gvspi/gvspi_net/gv701x_spi_net.ko ../bin/imx233_spi/
mv ../ghdd/ghdd.ko ../bin/imx233_spi/
mv ../cli/ghdd_cli_imx233.out ../bin/imx233_spi/
mv ../event-logger/ghdd_event_imx233.out ../bin/imx233_spi/

ls ../bin/imx233_spi/
