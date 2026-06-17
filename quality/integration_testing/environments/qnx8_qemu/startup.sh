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

echo "---> Starting slogger2"
slogger2 -s 65536
waitfor /dev/slog2

echo "---> Starting PCI Services"
pci-server --config=/proc/boot/pci_server.cfg
waitfor /dev/pci

echo "---> Starting Pipe"
pipe
waitfor /dev/pipe

echo "---> Starting Random"
random
waitfor /dev/random

echo "---> Starting fsevmgr"
fsevmgr
waitfor /dev/fsnotify

echo "---> Starting devb-ram"
devb-ram ram capacity=1 blk ramdisk=10m,cache=512k,vnode=256 2>/dev/null
waitfor /dev/ram0

echo "---> Mounting ram disk"
mkqnx6fs -q /dev/ram0
mount -t qnx6 /dev/ram0 /tmp_ram

echo "---> Starting mqueue"
mqueue
waitfor /dev/mqueue

echo "---> Starting devc-pty"
devc-pty -n 32

echo "---> Configuring network (static IP)"
/etc/network_setup.sh

echo "---> Setting hostname"
if [ -f /etc/hostname ]; then
    hostname "$(cat /etc/hostname)"
fi

echo "---> Adding /tmp_discovery folder"
mkdir -p /tmp_ram/tmp_discovery

/proc/boot/sshd -f /var/ssh/sshd_config
