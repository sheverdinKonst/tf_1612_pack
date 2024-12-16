#!/usr/bin/lua

local hash_cmd = "ubus call uci get '{\"config\":\"rpcd\"}'"

local tf = require "tf_module"
local configTable = {}

local function set_ip()
    --print("set_ip")
    --print("value = " .. configTable["IPADDRESS"])
    local cmd = "/etc/tf_clish/scripts/ipif/set_ipif.lua ifname  lan  proto static ip" .. configTable["IPADDRESS"]
    tf.executeCommand(cmd)
end

local function set_mask()
    --print("set_mask")
    --print("value = " .. configTable["NETMASK"])
    local cmd = "/etc/tf_clish/scripts/ipif/set_ipif.lua ifname  lan  proto static netmask" .. configTable["NETMASK"]
    tf.executeCommand(cmd)
end

local function set_gate()
    --print("set_gate")
    --print("value = " .. configTable["GATEWAY"])
    local cmd = "/etc/tf_clish/scripts/ipif/set_ipif.lua ifname  lan  proto static gateway" .. configTable["GATEWAY"]
    tf.executeCommand(cmd)
end

local function set_sysName()
    --print("set_sysName")
end

local function set_location()
    --print("set_location")
end

local function set_snmpState()
    --print("set_snmpState")
end

local function set_lldpState()
    --print("set_lldpState")
end

local keys_configTable =
{
    {param = "IPADDRESS",       f = set_ip        },
    {param = "NETMASK",         f = set_mask      },
    {param = "GATEWAY",         f = set_gate      },
    {param = "SYSTEM_NAME",     f = set_sysName   },
    {param = "SYSTEM_LOCATION", f = set_location  },
    {param = "SNMP_STATE",      f = set_snmpState },
    {param = "LLDP_STATE",      f = set_lldpState }
}

local function check_valid_hash()
    local res = "hash_error"
    local userdata = tf.collectJsonTable(hash_cmd)
    userdata = userdata["values"]
    if arg[1] ~= nil and arg[1] ~= "" then
        for _, user in pairs(userdata) do
            if user[".type"] == "login" then
                if user["hash"] ~= nil then
                    if user["hash"] == arg[1] then
                        res = "hash_ok"
                    end
                else
                    -- print("user name = " .. user["username"] .. " NOT HAVE HASH")
                end
            end
        end
    else
        res = "hash_not_valid"
    end
    return res
end

local function getSettingTable()
    local res = "msg_ok"
    if arg[2] ~= nil and arg[2] ~= "" then
        local parts = {}
        for part in string.gmatch(arg[2], "#([^#]+)") do
            table.insert(parts, part)
        end
        for _, part in ipairs(parts) do
            local start_index = string.find(part, "%[") + 1
            local end_index = string.find(part, "%]") - 1
            local key = string.sub(part, 1, start_index - 2)
            key = string.gsub(key, "=", "")
            local value = string.sub(part, start_index, end_index)
            configTable[key] = value
        end
    else
        res = "msg_error"
    end
    return res
end

local function setConfig()
    local res = "config_ok"
    if configTable ~= nil then
        for _, key in pairs(keys_configTable) do
            if configTable[key.param] ~= nil then
                --print("key.param = " .. key.param)
                key.f()
            end
        end
    else
        res = "config_error"
    end
    return res
end

local function run_hash_test()
    local hash_errorCode = "hash_error"
    local msg_errorCode = "msg_error"
    local config_error = ""

    hash_errorCode = check_valid_hash()
    if hash_errorCode == "hash_ok" then
        msg_errorCode = getSettingTable()
        if msg_errorCode == "msg_ok" then
            config_error = setConfig()
            print(config_error)
        else
            print(msg_errorCode)
        end
    else
        print(hash_errorCode)
    end
end

run_hash_test()

