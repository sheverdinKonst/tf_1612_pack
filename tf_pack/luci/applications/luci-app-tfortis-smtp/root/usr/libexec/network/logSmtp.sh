#!/bin/sh
read log
printf "Subject: $1\n\n$log" | msmtp -C/var/etc/msmtprc -a default $2
exit 0
