cd ../gvspi/gvspi_intf
make TARGET=imx287
cp -f Module.symvers ../gvspi_net/
cp -f modules.order ../gvspi_net/
cd -

cd ../gvspi/gvspi_net
make TARGET=imx287
cp -f Module.symvers ../../ghdd/
cp -f modules.order ../../ghdd/
cd -


cd ../ghdd
make TARGET=imx287
cd -

cd ../cli
make TARGET=imx287
cd -

cd ../event-logger
make TARGET=imx287
cd -

rm -rf ../bin/imx287_spi/gv701x_spi_intf.ko
rm -rf ../bin/imx287_spi/gv701x_spi_net.ko
rm -rf ../bin/imx287_spi/ghdd.ko
rm -rf ../bin/imx287_spi/ghdd_cli_imx287.out
rm -rf ../bin/imx287_spi/ghdd_event_imx287.out

mkdir -p ../bin/imx287_spi

mv ../gvspi/gvspi_intf/gv701x_spi_intf.ko ../bin/imx287_spi/
mv ../gvspi/gvspi_net/gv701x_spi_net.ko ../bin/imx287_spi/
mv ../ghdd/ghdd.ko ../bin/imx287_spi/
mv ../cli/ghdd_cli_imx287.out ../bin/imx287_spi/
mv ../event-logger/ghdd_event_imx287.out ../bin/imx287_spi/

ls ../bin/imx287_spi/
