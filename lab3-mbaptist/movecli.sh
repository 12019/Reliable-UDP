#! /bin/bash

if [ ! -d "clientdir" ]; then
     # Control will enter here if $DIRECTORY exists.
     mkdir clientdir;
     echo "clientdir directory created.\n"
fi

mv client ./clientdir
echo "client binary exec moved to clientdir directory.\n"

cp server-info.txt ./clientdir
echo "copied server-info.txt to clientdir.\n"
