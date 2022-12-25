#!/bin/bash
PIDS=`ps -ef |grep carctrl |grep -v grep | awk '{print $2}'`
if [ "$PIDS" == "" ]; then
    echo "start carctrl process..."
    ./carctrl
fi
