#!/usr/bin/lua

---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin.
--- DateTime: 6/4/24 11:50 AM
---

package.path = "/etc/tf_clish/scripts/snmp/?.lua;" .. package.path

local snmp  = require "snmp_module"
local tf    = require "tf_module"

local snmp_config = {}
local cmd_set = "ubus call uci set '{\"config\":\"snmpd\", \"type\":\""
local cmd_set_1 = "\"values\":{"

local snmp_version =
{
    "v1",
    "v2c",
    "usm"
}

local function get_argList()
    local cmd_general_values    = ""
    local cmd_v_values    = ""
    local version       = ""
    local community     = ""
    for i = 1, #arg do
        local p = arg[i]
        if snmp.snmp_argList[p] ~= nil then
            if p == "state" then
                local value = arg[i + 1]
                cmd_general_values = "\"enabled\":\"" .. value .. "\""
            elseif p == "port" then
                cmd_general_values = cmd_general_values
            elseif p == "version" then
                version = arg[i + 1]
                cmd_v_values = "\"version\":\"" .. version .. "\", \"version_state\":\"" .. arg[i + 2] .. "\""
                if version == "v3" then
                    local pass_error = 0
                    for i3 = i, #arg do
                        local p3 = arg[i3]
                        if snmp.snmp_v3_argList ~= nil then
                            if p3 == "username" then
                                cmd_v_values = cmd_v_values .. ", " .. "\"username\":\"" .. arg[i3 + 1] .. "\""
                            end
                            if p3 == "allow_write" then
                                cmd_v_values = cmd_v_values .. ", " .. "\"allow_write\":\"" .. arg[i3 + 1] .. "\""
                            end
                            if p3 == "auth_type" then
                                if arg[i3 + 1] ~= "none" then
                                    pass_error = 1
                                end
                                cmd_v_values = cmd_v_values .. ", " .. "\"auth_type\":\"" .. arg[i3 + 1] .. "\""
                            end
                            if p3 == "auth_pass" then
                                pass_error = pass_error - 1
                                cmd_v_values = cmd_v_values .. ", " .. "\"auth_pass\":\"" .. arg[i3 + 1] .. "\""
                            end
                            if p3 == "privacy_type" then
                                if arg[i3 + 1] ~= "none" then
                                    pass_error = pass_error + 1
                                end
                                cmd_v_values = cmd_v_values .. ", " .. "\"privacy_type\":\"" .. arg[i3 + 1] .. "\""
                            end
                            if p3 == "privacy_pass" then
                                pass_error = pass_error - 1
                                cmd_v_values = cmd_v_values .. ", " .. "\"privacy_pass\":\"" .. arg[i3 + 1] .. "\""
                            end
                        end
                    end
                    if pass_error ~= 0 then
                        print("error: no password for snmp v3 auth type or privacy type")
                        return
                    end
                    break
                end
            elseif p == "com" then
                community = arg[i + 1]
                local comm_name = arg[i + 2]
                cmd_v_values = cmd_v_values .. ", " .. "\"" .. community .. "\":\"" .. comm_name .. "\""
            elseif p == "source" then
                local access = snmp.access_struct[community]
                local source = arg[i + 1]
                if source ~= "custom" then
                    cmd_v_values = cmd_v_values .. ", " .. "\"" .. access .. "\":\"" .. source .. "\""
                elseif source == "custom" then
                    print("source == \"custom\"")
                    local ip_adddr = arg[i + 2]
                    local mask = ""
                    if arg[i + 3] ~= nil then
                        mask = arg[i + 3]
                    end
                    print("ip = " .. ip_adddr .. " mask = " .. mask)
                    cmd_v_values = cmd_v_values .. ", " .. "\"" .. access .. "\":\"" .. ip_adddr .. mask .. "\""
                end
            end
        end
    end
    local cmd_general = cmd_set .. "general\"," .. cmd_set_1 .. cmd_general_values .. "}}'"
    print("cmd_general = ")
    print(cmd_general)
    tf.executeCommand(cmd_general)

    local cmd_v = cmd_set .. "snmp_" .. version .. "\", " .. cmd_set_1 .. cmd_v_values .. "}}'"
    print("cmd_v = ")
    print(cmd_v)
    tf.executeCommand(cmd_v)
end

local function snmp_setSate(state)
    local section_type = "general"
    local cmd = cmd_set .. section_type .. "\", " .. cmd_set_1 .. "enabled\":\"" .. state .. "\"}}'"
    print("cmd = " .. cmd)
    tf.executeCommand(cmd)
end

local function run_config_snmp()
    if #arg < 2 then
        print("error: command is wrong")
        return
    end
    snmp.get_config()
    if arg[1] == "state" then
        if arg[2] == "0" then
            snmp_setSate(arg[2])
            return
        elseif arg[2] == "1" then
            if snmp.snmp_config.general.state ~= arg[2] then
                snmp_setSate(arg[2])
            end
            get_argList()
        end
    end
end

run_config_snmp()
