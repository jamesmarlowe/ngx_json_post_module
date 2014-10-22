#!/bin/bash

clear

echo "stopping nginx"
sudo /usr/local/nginx/sbin/nginx -s stop -p `pwd`
sudo killall nginx

echo "removing nginx logs"
sudo rm logs/*.log

echo "starting nginx from nginx.conf"
sudo /usr/local/nginx/sbin/nginx -c nginx.conf -p `pwd`
