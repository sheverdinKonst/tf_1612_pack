#!/bin/sh
printf "Subject: $1\n\n$2" | msmtp -C/var/etc/msmtprc -a default $3