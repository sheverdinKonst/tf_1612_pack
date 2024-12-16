#!/usr/bin/lua


package.path = "/etc/tf_clish/scripts/stp/?.lua;" .. package.path

local uci = require("luci.model.uci").cursor()
local tf = require "tf_module"
local stp = require "stp_module"

local function configState()
    local value = tonumber(arg[2])
    if value == 0 or value == 1 then
        uci:set("network", "switch", "stp", tostring(arg[2]))
    else
        io.write("error >> State value not corrected\n")
    end
end

local function configLogLevel()
    local value = tonumber(arg[2])
    if value >= 0 and value <= 4 then
        uci:set("mstpd", "global", "loglevel", tostring(arg[2]))
    else
        io.write("error >> Log level not corrected\n")
    end
end

local function configPriority()
    local value = tonumber(arg[2])
    if value >= 0 and value <= 15 then
        uci:set("mstpd", "switch", "treeprio", tostring(arg[2]))
    else
        io.write("error >> Priority value not corrected\n")
    end
end

local function configProtocol()
    if arg[2] == "stp" or arg[2] == "rstp" then
        uci:set("mstpd", "switch", "forcevers", tostring(arg[2]))
    else
        io.write("error >> Protocol value not corrected\n")
    end
end

local function configAgeing()
    local value = tonumber(arg[2])
    if value >= 10 and value <= 1000000 then
        uci:set("mstpd", "switch", "ageing", tostring(arg[2]))
    else
        io.write("error >> Ageing value must be in range 10 – 1.0000.00 seconds\n")
    end
end

local function configHello_time()
    local value = tonumber(arg[2])
    if value >= 1 and value <= 10 then
        uci:set("mstpd", "switch", "hello", tostring(arg[2]))
    else
        io.write("error >> Hello time must be in range 1 – 10 seconds\n")
    end
end

local function configForward_delay()
    local value = tonumber(arg[2])
    if value >= 4 and value <= 30 then
        uci:set("mstpd", "switch", "fdelay", tostring(arg[2]))
    else
        io.write("error >> Forward delay time must be in range 4 - 30 seconds\n")
    end
end

local function configMax_age()
    local value = tonumber(arg[2])
    if value >= 6 and value <= 40 then
        uci:set("mstpd", "switch", "maxage", tostring(arg[2]))
    else
        io.write("error: Max age time must be in range 6 - 40 seconds\n")
    end
end

local function configTX_hold_count()
    local value = tonumber(arg[2])
    if value >= 1 and value <= 10 then
        uci:set("mstpd", "switch", "txholdcount", tostring(arg[2]))
    else
        io.write("error: TX hold count must be in range 1 - 10 seconds\n")
    end
end

local stp_cmdList =
{
    {cmd = stp.cmd_paramList.state,         func = configState },
    {cmd = stp.cmd_paramList.loglevel,      func = configLogLevel },
    {cmd = stp.cmd_paramList.priority,      func = configPriority },
    {cmd = stp.cmd_paramList.protocol,      func = configProtocol },
    {cmd = stp.cmd_paramList.ageing,        func = configAgeing },
    {cmd = stp.cmd_paramList.hellotime,     func = configHello_time },
    {cmd = stp.cmd_paramList.forwarddelay,  func = configForward_delay },
    {cmd = stp.cmd_paramList.maxage,        func = configMax_age },
    {cmd = stp.cmd_paramList.txholdcount,   func = configTX_hold_count }
}

local function run_config_stp()
    for _, value in pairs(stp_cmdList) do
        if arg[1] == value.cmd then
            value.func()
        end
    end
end

run_config_stp()


