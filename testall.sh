#!/bin/bash

clear

# Barrier
echo "BarrierTest mit 10 Prozessen | sleep 0"
sleep 10
echo "gestartet"
./build/bin/osmp 10 -L ./logs/ -v 3 ./build/bin/osmpexecutable 1
echo "beendet"
sleep 10
clear

# Broadcast
echo "BroadcastTest mit 400 Prozessen | sleep 0"
sleep 10
echo "gestartet"
./build/bin/osmp 400 -L ./logs/ -v 3 ./build/bin/osmpexecutable 2
echo "beendet"
sleep 10
clear

# sendIRecv mit OSMP_Test
echo "sendIrecvTest mit 2 Prozessen | Send sleep(15) - Recv sleep(0)"
sleep 10
echo "gestartet"
./build/bin/osmp 2 -L ./logs/ -v 3 ./build/bin/osmpexecutable 3
echo "beendet"
sleep 10
clear

# Datatype Test
echo "DatatypeTest mit 2 Prozessen | Send sleep(0) - Recv sleep(3)"
sleep 10
echo "gestartet"
./build/bin/osmp 2 -L ./logs/ -v 3 ./build/bin/osmpexecutable 4
echo "beendet"
sleep 10
clear

# SendRecv mit mehr als 20 Nachrichten bevor gelesen wird
echo "SendRecvFull mit 2 Prozessen | Send sleep(0) - Recv sleep(2)"
sleep 10
echo "gestartet"
./build/bin/osmp 2 -L ./logs/ -v 3 ./build/bin/osmpexecutable 5
echo "beendet"
sleep 10
clear

# ISendIRecv
echo "IsendIRecv mit 2 Prozessen | Send sleep(10) - Recv sleep(20)"
sleep 10
echo "gestartet"
./build/bin/osmp 2 -L ./logs/ -v 3 ./build/bin/osmpexecutable 6
echo "beendet"
sleep 10
clear

# SendRecv
echo "SendRecv mit 2 Prozessen | Send sleep(0) - Recv sleep(2)"
sleep 10
echo "gestartet"
./build/bin/osmp 2 -L ./logs/ -v 3 ./build/bin/osmpexecutable 7
echo "beendet"
sleep 10
clear

# GetShmName
echo "getSHMName mit 2 Prozessen | sleep(0)"
sleep 10
echo "gestartet"
./build/bin/osmp 2 -L ./logs/ -v 3 ./build/bin/osmpexecutable 8
echo "beendet"
sleep 10
clear

# SendRecv mit mehr als Nachrichten als max_message_size erlaubt
echo "SendRecvNeighbour mit 500 Prozessen | Send sleep(0) - Recv sleep(20)"
sleep 10
echo "gestartet"
./build/bin/osmp 500 -L ./logs/ -v 3 ./build/bin/osmpexecutable 9
echo "beendet"
sleep 10
clear

