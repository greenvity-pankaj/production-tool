cd ../ghdd
make TARGET=imx233
cd -

cd ../cli
make TARGET=imx233
cd -

cd ../event-logger
make TARGET=imx233
cd -

rm -rf ../bin/imx233_eth/ghdd.ko
rm -rf ../bin/imx233_eth/ghdd_cli_imx233.out

mkdir -p ../bin/imx233_eth

mv ../ghdd/ghdd.ko ../bin/imx233_eth/
mv ../cli/ghdd_cli_imx233.out ../bin/imx233_eth/
mv ../event-logger/ghdd_event_imx233.out ../bin/imx233_eth/

ls ../bin/imx233_eth/


