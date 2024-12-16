#!/usr/bin/lua

---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin
--- DateTime: 6/6/24 6:25 PM
---

local tf = require "tf_module"

local cmd_get_user_config = "ubus call uci get '{\"config\":\"rpcd\"}'"
local cmd_add_user = "ubus call tf_system createUser '{\"username\":\""
local cmd_genHash = "ubus call tf_system generateHash '{\"username\":\""
local cmd_delete_user = "ubus call tf_system deleteUser '{\"username\":\""
local cmd_add_login = "ubus call uci add '{\"config\":\"rpcd\",\"type\":\"login\"}'"
local cmd_delete_section = "ubus call uci delete '{\"config\":\"rpcd\",\"section\":\""

local adminRule =
{
    full = "\"read\":[\"*\"], \"write\":[\"*\"]}}'",
    user = "\"read\":[\"*\"]}}'"
}

local function add_user()
    local user_info = tf.collectJsonTable(cmd_get_user_config)
    user_info = user_info["values"]
    for _, list in pairs(user_info) do
        if list[".type"] ~= nil and list[".type"] == "login" then
            if list["username"] == arg[2] then
                print("error: User *" .. arg[2] .. "* is present")
                return 0
            end
        end
    end

    local res = tf.collectJsonTable(cmd_add_login)
    local section = res["section"]

    local cmd_add = ""
    local cmd_hash = ""
    local cmd_config = ""
    local rule = arg[4]
    if arg[4] == "full" then

        cmd_add = cmd_add_user .. arg[2] .. "\", \"password\":\"" .. arg[3] .. "\", \"level\":\"" .. arg[4] .. "\"}'"
        cmd_hash = cmd_genHash .. arg[2] .. "\", \"password\":\"" .. arg[3] .. "\", \"level\":\"" .. arg[4] .. "\"}'"
        tf.executeCommand(cmd_add)
        local hash_value = tf.executeCommand(cmd_hash)
        hash_value = tf.decodeToJson(hash_value)
        hash_value = hash_value["result"]
        cmd_config = "ubus call uci set '{\"config\":\"rpcd\",\"type\":\"login\",\"section\":\"" .. section ..
            "\", \"values\":{\"username\":\"" .. arg[2] .. "\", \"password\":\"" .. "$p$" .. arg[2] .. "\", \"hash\":\"" .. hash_value .. "\"," .. adminRule[rule]     --tf.executeCommand(cmd_config)
        tf.executeCommand(cmd_config)
    elseif arg[4] == "user" then
        cmd_add = cmd_add_user .. arg[2] .. "\", \"password\":\"" .. arg[3] .. "\", \"level\":\"" .. arg[4] .. "\"}'"
        cmd_config = "ubus call uci set '{\"config\":\"rpcd\",\"type\":\"login\",\"section\":\"" .. section ..
            "\", \"values\":{\"username\":\"" .. arg[2] .. "\", \"password\":\"" .. "$p$" .. arg[2] .. "\"," .. adminRule[rule]
        tf.executeCommand(cmd_add)
        tf.executeCommand(cmd_config)
    end
end

local function delete_user()
    local cmd_config = ""
    local cmd_system = ""
    local user_info = tf.collectJsonTable(cmd_get_user_config)
    user_info = user_info["values"]
    local status = "not found"
    local count = 0
    for id, list in pairs(user_info) do
        if list[".type"] ~= nil and list[".type"] == "login" then
            if list["username"] == arg[2] then
                print("info: User *" .. arg[2] .. "* was deleted")
                cmd_config = cmd_delete_section .. id .. "\"}'"
                cmd_system = cmd_delete_user .. arg[2] .. "\"}'"
                tf.executeCommand(cmd_config)
                tf.executeCommand(cmd_system)
                status = "found"
                count = count + 1
            end
        end
        if count > 1 then
            print("error: user *" .. arg[2] .. "* more then 1")
        end
    end

    if status == "not found" then
        print("error: user *" .. arg[2] .. "* not found")
    end
end

local usr_cmdList =
{
    { cmd = "-a", func = add_user },
    { cmd = "-d", func = delete_user },
}

local function run_config_user()
    for _, value in pairs(usr_cmdList) do
        if arg[1] == value.cmd then
            value.func(arg[3])
        end
    end
end

run_config_user()
