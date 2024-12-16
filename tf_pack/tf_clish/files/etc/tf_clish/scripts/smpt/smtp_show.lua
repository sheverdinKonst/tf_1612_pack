#!/usr/bin/lua

---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin.
--- DateTime: 8/12/24 4:06 PM
---

local tf = require "tf_module"

local smtp_ubus_config  = "ubus call uci get '{\"config\":\"smtp\"}'"

local smtp_list = {
    { name = "enabled",     str = "\tstate:         " },
    { name = "tls",         str = "\tuse TLS:       " },
    { name = "port",        str = "\tport:          " },
    { name = "host",        str = "\tSMTP host:     " },
    { name = "subject",     str = "\tmail subject:  " },
    { name = "user",        str = "\tuser:          " },
    { name = "password",    str = "\tpassword:      " },
    { name = "to1",         str = "\treceiver 1:    " },
    { name = "to2",         str = "\treceiver 2:    " },
    { name = "to3",         str = "\treceiver 3:    " },
}

local function run_smtp_show()
    local smtpConfig_jsonInfo = tf.collectJsonTable(smtp_ubus_config)
    smtpConfig_jsonInfo = smtpConfig_jsonInfo.values
    for _, smtp_dict in pairs(smtpConfig_jsonInfo) do
        for _, row in pairs(smtp_list) do
            if smtp_dict[row.name] ~= nil then
                if row.name ~= "password" then
                    print(row.str .. smtp_dict[row.name])
                end

                if row.name == "user" then
                    if arg[1] == "password" then
                        if smtp_dict["password"] ~= nil then
                            print(smtp_list[7].str .. smtp_dict["password"])
                        else
                            print("error: password not defined by user")
                        end
                    end
                end
            end
        end
    end
end

local function callTestEmail()
    -- local cmd_sendMail = "/usr/libexec/network/sendSmtp.sh"
    local cmd_sendMail = "ubus call file exec '{"
    local cmd_command = "\"command\":\"/usr/libexec/network/sendSmtp.sh\", \"params\": ["
    local params =
    {
        subject = "\"",
        text_field = "\"TFortis test email\", ",
        to = "\""
    }

    local smtpConfig_jsonInfo = tf.collectJsonTable(smtp_ubus_config)
    smtpConfig_jsonInfo = smtpConfig_jsonInfo.values
    for _, smtp_dict in pairs(smtpConfig_jsonInfo) do
        local to_count = 0
        if smtp_dict["subject"] ~= nil then
            params.subject = params.subject .. smtp_dict["subject"] .. "\", "
        else
            print("info: mail without \"subject\"")
        end
        if smtp_dict["to"] ~= nil then
            params.to = params.to .. smtp_dict["to"]
            to_count = to_count + 1
        end
        if smtp_dict["to1"] ~= nil then
            params.to = params.to .. smtp_dict["to1"]
            to_count = to_count + 1
        end
        if smtp_dict["to2"] ~= nil then
            params.to = params.to .. smtp_dict["to2"]
            to_count = to_count + 1
        end
        if smtp_dict["to3"] ~= nil then
            params.to = params.to .. smtp_dict["to3"]
            to_count = to_count + 1
        end
        params.to = params.to .. "\""
        cmd_sendMail = cmd_sendMail .. cmd_command .. params.subject .. params.text_field .. params.to .. "]}'"
        local res = ""
        res = tf.executeCommand(cmd_sendMail)
        if res == "" or res == nil then
            print("error: smtp command failed")
        else
            local res_json = tf.decodeToJson(res)
            if tonumber(res_json.code) ~= 0 then
                print("error: send test mail is failed: ")
                print(res_json.stderr)
            elseif tonumber(res_json.code) == 0 then
                print("info: message was sent, check your mail !!!")
            end
        end
    end
end


if arg[1] == "show_config" then
    run_smtp_show()
elseif arg[1] == "test" then
    callTestEmail()
end


