#!/bin/sh

tc qdisc add dev enp2s0 root netem loss 10% delay 10ms 3ms distribution normal
