#!/usr/bin/lua

---
--- Generated by Luanalysis
--- Created by sheverdin.
--- DateTime: 2/16/24 4:33 PM
---

package.path = "/etc/tf_clish/scripts/igmp/?.lua;" .. package.path

local tf            = require "tf_module"
local igmp          = require "igmp_module"
local igmp_status    = require "igmp_status"

local portRange = {}

local show_igmp_option =
{
    "status",
    "config"
}

local function igmp_show_status()
    igmp_status.main_show_igmp_status(portRange)
end

local function igmp_show_config()
    for i, option in pairs(igmp.option) do
        local ubusStr = tf.ubusList.cmd_ubus ..
                tf.ubusList.cmd_get ..
                tf.ubusList.cmd_prefix ..
                tf.ubusList.cmd_config .. "network" ..
                tf.ubusList.cmd_section .. "switch" ..
                tf.ubusList.cmd_option .. option ..
                tf.ubusList.cmd_suffix
        local switchData = tf.collectJsonTable(ubusStr)
        if (switchData == nil) then
            io.write(igmp.printStr[i], "not configured\n")
        else
            io.write(igmp.printStr[i], switchData["value"], "\n")
        end
    end
end

local igmp_show_cmdList = {
    { cmd = "status", func = igmp_show_status },
    { cmd = "config", func = igmp_show_config },
}

local function show_igmp(cmdList)
    for _, value in pairs(cmdList) do
        if arg[1] == value.cmd then
            error = value.func()
        end
    end
end

local function main_show_igmp()
    portRange = tf.checkPortRange(arg[2])
    if (portRange ~= nil) then
        show_igmp(igmp_show_cmdList)
    end
end

main_show_igmp()

