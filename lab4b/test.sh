#!/bin/bash  

echo "--------Smoke Test-------"

./lab4b --period=2 --scale=C --log="LOGFILE" <<-EOF
STOP
SCALE=F
PERIOD=3
START
LOG test
OFF
EOF
ret=$?
if [ $ret -ne 0 ]
then
        echo "RETURNS RC=$ret"
        echo "Failed. Incorrect return code"
fi

if [ ! -s LOGFILE ]
then
        echo "Failed. Did not create a log file"
else
        for c in SCALE=F PERIOD=3 START STOP OFF SHUTDOWN "LOG test"
        do
                grep "$c" LOGFILE > /dev/null
                if [ $? -ne 0 ]
                then
                        echo "DID NOT LOG $c command"
                else
                        echo "    $c ... RECOGNIZED AND LOGGED"
                fi
        done
fi
rm LOGFILE
