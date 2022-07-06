#!/bin/bash

start_led_blink ()
{
    trap 'echo 0 > /dev/led-lkm ;  exit 1' SIGINT

    while true
    do
        echo 1 > /dev/led-lkm
        sleep 0.5
        echo 0 > /dev/led-lkm
        sleep 0.5
   done
}

stop_led_blink ()
{
    kill -TERM $!
    echo 0 > /dev/led-lkm
    sleep 0.1
}

echo "check_led: test started: "

start_led_blink&

while true
do
    read -n 1 -p "${1:-is the LED blinking?} [y(Y)/n(N)]: " REPLY
    echo
    case ${REPLY} in
        [yY] )
            echo "check_led: the test passed"
            stop_led_blink
            exit 0
            ;;
        [nN] )
            echo "check_led: the test failed"
            stop_led_blink
            exit 1
            ;;
        *)
            echo "check_led: only y(Y) or n(N) are needed for a reply!    (CTRL + C for exit)"
            ;;
    esac
done
