#!/bin/bash
# please check help_usage for command guide

function help_usage()
{
    echo "command guide:"
    echo "execute.sh -r -s -h"
    echo "    -r|--run:   run all applications"
    echo "    -s|--stop:  stop all applications"
    echo "    -h|--help:  help"
}

app_list="agvctrl agvintf"

#set all application to run or stop
function set_application_state()
{
    local state=$1

    cd /opt/build.carctrl
    for item in $app_list; do
        local pids=$(ps -ef |grep $item |grep -v grep | awk '{print $2}')
        if [ "$pids" == "" ]; then #app not run
            if [ $state -eq 1 ]; then
                echo "start application: $item..."
                ./$item &
            fi
        else #app already run
            if [ $state -eq 0 ]; then
                echo "stop application: $item..."
                kill -9 $pids
            fi
        fi
    done
}

#main function
while [[ $# -gt 0 ]]; do
    key=$1
    case $key in
        -s|--stop) #stop application
            set_application_state 0
            exit 0
            ;;
        -h|--help)
            help_usage
            exit 0
            ;;
        *) #default to run application
            set_application_state 1
            exit 0
            ;;
    esac
done
