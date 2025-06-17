#!/bin/sh

if [ $(id -u) -ne 0 ]; then
    echo 'You need to be root to setup the server!' && exit 1
fi

mkdir -p /srv/http/player

cp -rf ./config-httpserver/* /srv/http/
chmod 666 /srv/http/*.txt
chmod 777 /srv/http/player
