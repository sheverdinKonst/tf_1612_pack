#!/usr/bin/lua

package.path = "/etc/tf_clish/scripts/stp/?.lua;" .. package.path

local tf = require "tf_module"
local stp = require "stp_module"

local values_t = {
    values_e        = "values",
    global_e        = "global",
    switch_e        = "switch",
    switch_ports_e  = "ports"
}

local global_loglevel_e = {
    "disable",
    "error",
    "info",
    "debug",
    "state machine transition"
}

local global_enabled = {
    "no",
    "yes"
}

local switch_forcevers = {
    "STP",
    "RSTP",
    "MSTP"
}

local switch_treeprio = {
    "0",
    "4096",
    "8192",
    "12288",
    "16384",
    "20480",
    "24576",
    "28672",
    "32768",
    "36864",
    "40960",
    "45056",
    "49152",
    "53248",
    "57344",
    "61440",
}

local mstpd_config = {
    global_config =
    {
        {".name",       ""},
        {"enabled",     "\tEnabled:         " },
        {"loglevel",    "\tLoglevel:        " },
        {"bridge",      "\tBridge list:\n   " }
    },
    switch_config =
    {
        {".name",       "" },
        {".type",       "\tDevice type:     " },
        {"forcevers",   "\tProtocol:        " },
        {"treeprio",    "\tPriority:        " },
        {"hello",       "\tHello time:      " },
        {"fdelay",      "\tFw delay time:   " },
        {"maxage",      "\tMax age:         " },
        {"ageing",      "\tAgeing:          " },
        {"txholdcount", "\tTX hold count:   " },
    },
    lan_config =
    {
        {".name",      "" },
        {".type",      "\tType:             " },
        {"treeprio",   "\tPriority:         " },
        {"pathcost",   "\tPath  cost:       " },
        {"adminedge",  "\tAdmin edge:       " },
        {"autoedge",   "\tAuto  edge:       " },
        {"bpdufilter", "\tBPDU  filter:     " },
        {"bpduguard",  "\tBPDU  guard:      " }
    }
}

local function printStpConfigGlobal(mstpdConfigValues)
    local mstpd_global_list = mstpdConfigValues[values_t.global_e]
    local mstpd_switch_list = mstpdConfigValues[values_t.switch_e]

    for _, value_g in pairs(mstpd_config.global_config) do

        if mstpd_global_list[value_g[1]] ~= nil then
            if value_g[1] == ".name" then
                local name = tf.capFirstLetter(mstpd_global_list[value_g[1]])
                io.write(name, ":\n")
            elseif value_g[1] == "enabled" then
                local enabledIndex = mstpd_global_list[value_g[1]]
                local enabled_v = global_enabled[enabledIndex + 1]
                io.write(value_g[2], enabled_v, "\n")
            elseif value_g[1] == "loglevel" then
                local loglevelIndex = mstpd_global_list[value_g[1]]
                local loglevel_v = global_loglevel_e[loglevelIndex + 1]
                io.write(value_g[2], loglevel_v, "\n")
            elseif value_g[1] == "bridge" then
                io.write(value_g[2])
                for i,j in pairs(mstpd_global_list[value_g[1]]) do
                    io.write("\t\t", j, "\n")
                end
            end
        end
    end

    for _, value_sw in pairs(mstpd_config.switch_config) do
        if mstpd_switch_list[value_sw[1]] ~= nil then
            if value_sw[1] == ".name" then
                local name = tf.capFirstLetter(mstpd_switch_list[value_sw[1]])
                io.write(name, ":\n")
            elseif value_sw[1] == "treeprio"  then
                local treeprioIndex = mstpd_switch_list[value_sw[1]]
                local treeprio_v = switch_treeprio[treeprioIndex + 1]
                io.write(value_sw[2], treeprio_v, "\n")
            else
                io.write(value_sw[2], mstpd_switch_list[value_sw[1]], "\n")
            end
        end
    end
end

local function printStpConfigPort(portList, range, mstpdConfig)
    for _, portStr in pairs(portList) do
        local port, _ = tf.getPort(portStr)
        port = tonumber(port)
        local minPort  = tonumber(range[1])
        local maxPort  = tonumber(range[2])
        if port >= minPort and port <= maxPort then
            local mstpdPortConfig = mstpdConfig[portStr]
            for _, value_lan in pairs(mstpd_config.lan_config) do
                if value_lan[1] == ".name" then
                    local name = tf.capFirstLetter(mstpdPortConfig[value_lan[1]])
                    io.write("-----------------------------------------------\n")
                    io.write(name, ":\n")
                elseif value_lan[1] == "treeprio"  then
                    local treeprioIndex = mstpdPortConfig[value_lan[1]]
                    local treeprio_v    = switch_treeprio[treeprioIndex + 1]
                    io.write(value_lan[2], treeprio_v, "\n")
                else
                    io.write(value_lan[2], mstpdPortConfig[value_lan[1]], "\n")
                end
            end
        end
    end
end

local function run_stp_show_config()
    local cmd_in        = arg[1]
    local portRange_in  = arg[2]
    local portRange     = {}
    local portList      = {}
    local mstpdConfig   = {}
    local netConfig     = {}

    local errorCode = 0;

    portRange = tf.checkPortRange(portRange_in)
    if portRange == nil then
        print("error: stp port range is wrong\n")
        errorCode = 1
    end

    if errorCode == 0 then
        netConfig = tf.collectJsonTable(stp.cmdList.get_netConfig)
        if netConfig == nil then
            print("error: net config is empty\n")
            errorCode = 1
        end

        mstpdConfig = tf.collectJsonTable(stp.cmdList.get_mstpdConfig)
        if mstpdConfig == nil then
            print("error: MSTPD config is empty\n")
            errorCode = 1
        end

        portList = tf.getPortList(netConfig[values_t.values_e], values_t.switch_e, values_t.switch_ports_e)
        if portList == nil then
            print("error: port in switch device List is empty\n")
            errorCode = 1
        end
    end

    if errorCode == 0 then
        if cmd_in == "bridge" then
            printStpConfigGlobal(mstpdConfig[values_t.values_e])
        elseif cmd_in == "ports" then
            printStpConfigPort(portList, portRange, mstpdConfig[values_t.values_e])
        else
            print("error: stp command not valid")
        end
    end
end

run_stp_show_config()

