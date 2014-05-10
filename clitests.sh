#! /bin/bash
set -x
#---- FAILING TESTS-----#

# test 1
./client
echo STATUS: $?

#test 2
./client -s -x
echo STATUS: $?

#test 3
./client -s 129.3.9.0.0 12313213
echo STATUS: $?

#test 4
./client -x 129.0.0.1 12313d
echo STATUS: $?

#test 5 
./client -s 129.3.9.d0.0 1234
echo STATUS: $?

#test 6 
./client -x 129.3.9.d0.0 1234
echo STATUS: $?

#test 7
./client -s -x 129.3.9.d0.0 1234
echo STATUS: $?


#-----------------------#


#----PASSING TESTS------#

#test 8
echo "Make sure IP is correct and port"
./client 192.168.0.104 1257

echo grassword | ./client -s 192.168.0.104 1257

#-----------------------#
