#!/bin/bash

echo "Enter user password:"
read -s  password
echo "Password entered -->  " "${password}"

/etc/tf_clish/scripts/smpt/smtp_config.lua "user" "${1}" "password" "${password}"