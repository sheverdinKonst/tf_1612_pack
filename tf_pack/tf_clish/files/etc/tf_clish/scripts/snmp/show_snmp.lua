#!/usr/bin/lua

---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin.
--- DateTime: 7/29/24 4:21 PM
---

package.path = "/etc/tf_clish/scripts/snmp/?.lua;" .. package.path

local tf = require "tf_module"
local snmp  = require "snmp_module"

local state_enum =
{
    ["0"] = "disable",
    ["1"] = "enable"
}

local logic_enum =
{
    ["0"] = "no",
    ["1"] = "yes"
}

local function run_show_snmp()
    snmp.get_config()
    if snmp.snmp_config == nil then
        print("error: snmp config is wrong")
        return
    end
    if snmp.snmp_config.general == nil then
        print("error: snmp config is wrong")
        return
    end
    print("General:")
    print("\tstate: " .. snmp.snmp_config.general.state)

    if snmp.snmp_config["v1"] == nil then
        return
    end
    print("SNMP V1:")
    print("\tstate:             " .. state_enum[snmp.snmp_config["v1"].version_state])
    print("\tread_community:    " .. snmp.snmp_config["v1"].v1_v2_param.read_community)
    print("\t\taccess:          " .. snmp.snmp_config["v1"].v1_v2_param.read_access)
    print("\twrite_community:   " .. snmp.snmp_config["v1"].v1_v2_param.write_community)
    print("\t\taccess:          " .. snmp.snmp_config["v1"].v1_v2_param.write_access)

    if snmp.snmp_config["v2c"] == nil then
        return
    end
    print("SNMP V2C:")
    print("\tstate:             " .. state_enum[snmp.snmp_config["v2c"].version_state])
    print("\tread_community:    " .. snmp.snmp_config["v2c"].v1_v2_param.read_community)
    print("\t\taccess:          " .. snmp.snmp_config["v2c"].v1_v2_param.read_access)
    print("\twrite_community:   " .. snmp.snmp_config["v2c"].v1_v2_param.write_community)
    print("\t\taccess:          " .. snmp.snmp_config["v2c"].v1_v2_param.write_access)

    if snmp.snmp_config["v3"] == nil then
        return
    end
    print("SNMP V3:")
    print("\tstate:         " .. state_enum[snmp.snmp_config["v3"].version_state])
    print("\tuser name:     " .. snmp.snmp_config["v3"].username)
    print("\tallow write:   " .. logic_enum[snmp.snmp_config["v3"].allow_write])
    print("\tauth type:     " .. snmp.snmp_config["v3"].auth_type)
    print("\tauth pass:     " .. snmp.snmp_config["v3"].auth_pass)
    print("\tprivacy type:  " .. snmp.snmp_config["v3"].privacy_type)
    print("\tprivacy pass:  " .. snmp.snmp_config["v3"].privacy_pass)
end

run_show_snmp()



