#!/bin/bash

find /etc/tf_clish -name "*.lua" -exec chmod 0755 {} \;
find /usr/libexec/rpcd -name "*.*" -exec chmod 0755 {} \;