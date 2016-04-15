cd ../ghdd
make TARGET=x86
cd -

cd ../cli
make TARGET=x86
cd -

cd ../event-logger
make TARGET=x86
cd -

rm -rf ../bin/x86/ghdd.ko
rm -rf ../bin/x86/ghdd_cli_x86.out
rm -rf ../bin/x86/ghdd_event_x86.out

mkdir -p ../bin/x86
mv ../ghdd/ghdd.ko ../bin/x86/
mv ../cli/ghdd_cli_x86.out ../bin/x86/
mv ../event-logger/ghdd_event_x86.out ../bin/x86/

ls ../bin/x86


