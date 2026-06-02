#!/bin/sh

# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************


# *******************************************************************************
# Network Configuration Script
# Configures networking interfaces and settings for QNX system
# *******************************************************************************

echo "---> Starting Networking"
io-sock -m phy -m pci -d vtnet_pci
waitfor /dev/socket

echo "---> Configuring network interface"
if_up -p vtnet0
ifconfig vtnet0 10.0.2.15 netmask 255.255.255.0
ifconfig vtnet0 169.254.158.190 netmask 255.255.0.0

sysctl -w net.inet.icmp.bmcastecho=1 > /dev/null
sysctl -w qnx.sec.droproot=33:33 > /dev/null
