

local utils   = require "lldp_utils"
local chassis = require "lldp_chassis"
local tf      = require "tf_module"

local interface_module = {}

local i_portCount = 0

local interface_port_e = {
    id      = "id",
    descr   = "descr",
    id_t    = utils.id_e
}

local lldp_interface_e = {
    name    = "name",
    status  = "status",
    chassis = "chassis",
    port    = "port",
    ttl     = "ttl",
    port_t  = interface_port_e
}

function interface_module.parsingInterfaces(lldp_interface)
    --print("---> interfaces")
    print(" ---------------------------------- ")
    i_portCount = i_portCount + 1
    print(lldp_interface[lldp_interface_e.name] .. ":")

    for _, interface in pairs(lldp_interface[lldp_interface_e.status]) do
        print("\t" .. lldp_interface_e.status .. ": \t\t" .. interface["value"])
    end

    for key, interface in pairs(lldp_interface[lldp_interface_e.chassis]) do
        print("\tChassis: ")
        chassis.parsingChassis(lldp_interface)
    end

    print("\tPort: ")
    for _, interface_port in pairs(lldp_interface[lldp_interface_e.port]) do
        if interface_port ~= nil then
            local id_type   = lldp_interface_e.port_t.id_t.type
            local id_value  = lldp_interface_e.port_t.id_t.value
            local id = lldp_interface_e.port_t.id
            local descr = lldp_interface_e.port_t.descr
            if (interface_port[id] ~= nil) then
                for _, port_v in pairs(interface_port[id]) do
                    print("\t\tid:\t\t" .. port_v[id_type] .. "\t" .. port_v[id_value])
                end
            end

            if (interface_port[descr] ~= nil) then
                for _, descr_v in pairs(interface_port[descr])  do
                    print("\tDescription: " .. "\t" .. descr_v["value"] )
                end
            end
        end
    end

    for key, ttl_v in pairs(lldp_interface[lldp_interface_e.ttl]) do
        print("\t" .. lldp_interface_e.ttl .. ":\t" .. ttl_v[lldp_interface_e.ttl])
    end
    return i_portCount
end


return interface_module

