#!/bin/bash
PIDS=`ps -ef |grep test |grep -v grep | awk '{print $2}'`
if [ "$PIDS" == "" ]; then
    echo "start car process..."
    /srv/tftp/test &
fi
