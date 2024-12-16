#!/usr/bin/lua

---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin.
--- DateTime: 6/3/24 2:18 PM
---

local tf = require "tf_module"

local cmd_network_ubus = "ubus call uci get '{\"config\":\"network\", \"type\":\"interface\"}'"
local cmd_ubus_set = "ubus call uci set '{\"config\":\"network\",\"type\":\"interface\",\"section\":\""
local cmd_ubus_del = "ubus call uci delete '{\"config\":\"network\",\"type\":\"interface\",\"section\":\""

local ipif_struct =
{
    if_name     = "",
    proto       = "",
    IP_addr     = "",
    netmask_IP  = "",
    gateway_IP  = "",
    dns_IP      = "",
}

local ipif_listArg    = {}
local ipif_listConfig = {}

local ipif_paramList = {
    "ifname",
    "proto",
    "ip",
    "netmask",
    "gateway",
    "dns",
}

local function ipif_config()
    local section = ""

    local ubus_cmd = ""
    local ubus_set_2 = "\","

    local ubus_set_value = "\"values\":{"
    local ubus_set_value_1 = ""

    local count = 0
    local ubus_set_value_list = {}

    section = ipif_listArg.ifname

    if ipif_listArg.proto == "static" then
        count = count + 1
        ubus_set_value_list[count] = "\"proto\":\"" .. ipif_listArg.proto .. "\""

        if ipif_listArg.ip ~= "" and ipif_listArg.ip ~= nil then
            count = count + 1
            ubus_set_value_list[count] = "\"ipaddr\":\"" .. ipif_listArg.ip .. "\""
        end

        if ipif_listArg.netmask ~= "" and ipif_listArg.netmask ~= nil then
            count = count + 1
            ubus_set_value_list[count] = "\"netmask\":\"" .. ipif_listArg.netmask .. "\""
        end

        if ipif_listArg.gateway ~= "" and ipif_listArg.gateway ~= nil then
            count = count + 1
            ubus_set_value_list[count] = "\"gateway\":\"" .. ipif_listArg.gateway .. "\""
        end

        if ipif_listArg.dns ~= "" and ipif_listArg.dns ~= nil then
            count = count + 1
            ubus_set_value_list[count] = "\"dns\":\"" .. ipif_listArg.dns .. "\""
        end
        ubus_cmd = cmd_ubus_set .. section .. ubus_set_2 .. ubus_set_value
        if count == 1 then
            ubus_set_value_1 = ubus_set_value_1 .. ubus_set_value_list[1]
        elseif count > 1 then
            for j = 1, count - 1 do
                ubus_set_value_1 = ubus_set_value_1 .. ubus_set_value_list[j] .. ","
            end
            ubus_set_value_1 = ubus_set_value_1 .. ubus_set_value_list[count]
        end
        ubus_cmd = ubus_cmd .. ubus_set_value_1 .. "}}'"
        tf.executeCommand(ubus_cmd)
        --print("count = " .. count)
        --print("ubus_set_value_1  = " .. ubus_set_value_1)
        --print("ubus_cmd 2 = " .. log_ubus_cmd)
    elseif ipif_listArg.proto == "dhcp" then
        cmd_ubus_del = cmd_ubus_del .. section .. "\", \"options\":[\"ipaddr\", \"netmask\", \"gateway\", \"dns\"]}'"
        --print("cmd_ubus_del = " .. cmd_ubus_del)
        tf.executeCommand(cmd_ubus_del)
        cmd_ubus_set = cmd_ubus_set .. section .. "\", \"values\":{\"proto\":\"dhcp\"}}'"
        --print("cmd_ubus_set = " .. cmd_ubus_set)
        tf.executeCommand(cmd_ubus_set)
    end
end

local function set_ipif_param()

    local network_jsonInfo = tf.collectJsonTable(cmd_network_ubus)
    network_jsonInfo = network_jsonInfo.values

    ipif_listArg = tf.getparam(ipif_paramList, arg)

    if network_jsonInfo[ipif_listArg.ifname] == nil then
        print("error: interface name is wrong")
        return
    end

    if ipif_listArg.ifname == "loopback" then
        print("error: interface \"loopback\" can not be modified")
        return
    end

    ipif_config()

end

set_ipif_param()