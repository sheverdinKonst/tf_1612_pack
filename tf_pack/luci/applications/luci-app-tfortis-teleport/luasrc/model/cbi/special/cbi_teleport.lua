local fs = require "nixio.fs"
local sys = require "luci.sys"
require "ubus"
require "socket"
require "socket.unix"

m = Map("tfortis-teleport", translate("Teleport")) -- cbi_file is the config file in /etc/config
d = m:section(TypedSection, "tfortis-teleport")  -- info is the section called info in cbi_file
d.anonymous = true
d.addremove = false


d:tab("tab_status", translate("Status"))
d:tab("tab_settings", translate("Settings"))

-- Настройки
a = d:taboption("tab_settings",Value, "discharge_voltage", "Discharge voltage");

LogFileMaxSize = d:taboption("tab_settings", Value, "LogFileMaxSize", translate("Max size of log file"))
LogFileMaxSize:value("512K",  translate("512K"))
LogFileMaxSize:value("1M",  translate("1M"))
LogFileMaxSize:value("2M",  translate("2M"))
LogFileMaxSize.default = "1M"


-- Статусная информация
a=d:taboption("tab_status", DummyValue, "state", translate("UPS connected"))
a.rows = 1
a.rmempty = false
function a.cfgvalue()
    return luci.util.exec("/usr/bin/request_ctrl RPS_CONNECTED get")
end

a=d:taboption("tab_status", DummyValue, "state", translate("VAC Status"))
a.rows = 1
a.rmempty = false
function a.cfgvalue()
    return luci.util.exec("/usr/bin/request_ctrl RPS_VAC get")
end

a=d:taboption("tab_status", DummyValue, "state", translate("Battery voltage"))
a.rows = 1
a.rmempty = false
function a.cfgvalue()
    return luci.util.exec("/usr/bin/request_ctrl RPS_BAT_VOLTAGE get")
end

a=d:taboption("tab_status", DummyValue, "state", translate("Battery current"))
a.rows = 1
a.rmempty = false
function a.cfgvalue()
    return luci.util.exec("/usr/bin/request_ctrl RPS_BAT_CURRENT get")
end

a=d:taboption("tab_status", DummyValue, "state", translate("Temperature"))
a.rows = 1
a.rmempty = false
function a.cfgvalue()
    return luci.util.exec("/usr/bin/request_ctrl RPS_TEMPER get")
end

a=d:taboption("tab_status", DummyValue, "state", translate("UPS remain time"))
a.rows = 1
a.rmempty = false
function a.cfgvalue()
    return luci.util.exec("/usr/bin/request_ctrl RPS_REMAIN_TIME get")
end

return m;
