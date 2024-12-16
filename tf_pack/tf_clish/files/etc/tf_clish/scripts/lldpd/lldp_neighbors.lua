
local utils   = require "lldp_utils"
local tf      = require "tf_module"


local lldp_neighbors_module = {}

local n_portCount = 0

local neighbors_interface_e = {
    name            = "name",
    via             = "via",
    rid             = "rid",
    age             = "age",
    chassis         = "chassis",
    port            = "port",
    vlan            = "vlan",
    ppvid           = "ppvid"
}

local advertised_e = {
    type            = "type",
    hd              = "hd",
    fd              = "fd",
}

local auto_negotiation_e =
{
    supported       = "supported",
    enabled         = "enabled",
    advertised      = "advertised",
    current         = "current",
    advertised_t    = advertised_e,
}

local neighbors_port_e = {
    id                 = "id",
    descr              = "descr",
    ttl                = "ttl",
    mfs                = "mfs",
    aggregation        = "aggregation",
    auto_negotiation   = "auto-negotiation",
    id_t               = utils.id_e,
    auto_negotiation_t = auto_negotiation_e
}

local neighbors_vlan_e = {
    vlan_id            = "vlan-id",
    pvid               = "pvid",
}

local neighbors_ppvid_e = {
    supported           = "supported",
    enabled             = "enabled",
}

local neighbors_e = {
    name                = "neighbors",
    interface           = neighbors_interface_e,
    chassis             = utils.chassis_e,
    port                = neighbors_port_e,
    vlan                = neighbors_vlan_e,
    ppvid               = neighbors_ppvid_e
}

local function parsingNeighborsPort(neighbors_ports, port_e)
    print("Port: ")
    local id                = port_e.id
    local descr             = port_e.descr
    local ttl               = port_e.ttl
    local mfs               = port_e.mfs
    local aggregation       = port_e.aggregation
    local auto_negotiation  = port_e.auto_negotiation

    local id_type   = port_e.id_t.type
    local id_value  = port_e.id_t.value

    if (neighbors_ports[id] ~= nil) then
        for i, struct in pairs(neighbors_ports[id])  do
            print("\tid: " .. "\t\t" .. struct[id_type]  .. "\t" .. struct[id_value])
        end
    end

    if (neighbors_ports[descr] ~= nil) then
        for i, struct in pairs(neighbors_ports[descr])  do
            print("\tdescription: " .. "\t" .. struct["value"] )
        end
    end

    if (neighbors_ports[ttl] ~= nil) then
        for i, struct in pairs(neighbors_ports[ttl])  do
            print("\tttl: " .. "\t" .. struct["value"] )
        end
    end

    if (neighbors_ports[mfs] ~= nil) then
        for i, struct in pairs(neighbors_ports[mfs])  do
            print("\tmfs: " .. "\t" .. struct["value"] )
        end
    end

    if (neighbors_ports[aggregation] ~= nil) then
        for i, struct in pairs(neighbors_ports[aggregation])  do
            print("\taggregation: " .. "\t" .. struct["value"] )
        end
    end

    local supported     = neighbors_port_e.auto_negotiation_t.supported
    local enabled       = neighbors_port_e.auto_negotiation_t.enabled
    local current       = neighbors_port_e.auto_negotiation_t.current
    local advertised    = neighbors_port_e.auto_negotiation_t.advertised

    print("\tAuto-negotiation: " )
    if neighbors_ports[auto_negotiation] ~= nil then
        for i, s_auto_negotiation in pairs(neighbors_ports[auto_negotiation])  do

            print("\t\tsupported: " .. "\t" .. tostring(s_auto_negotiation[supported]))
            print("\t\tenabled: "   .. "\t" .. tostring(s_auto_negotiation[enabled]))

            if (s_auto_negotiation[current] ~= nil ) then
                for _, v_current in pairs(s_auto_negotiation[current])  do
                    print("\t\tcurrent: " .. "\t" .. v_current["value"] )
                end
            end

            local type = neighbors_port_e.auto_negotiation_t.advertised_t.type
            local fd   = neighbors_port_e.auto_negotiation_t.advertised_t.fd
            local hd   = neighbors_port_e.auto_negotiation_t.advertised_t.hd

            print("\t\tAdvertised: " )
            if (s_auto_negotiation[advertised] ~= nil ) then

                for _, struct_advertised in pairs(s_auto_negotiation[advertised])  do
                    print("\t\t      - - - - - - - - - - -")
                    if (struct_advertised[type] ~= nil ) then
                        print("\t\t\ttype: " .. "\t" .. tostring(struct_advertised[type]))
                    end

                    if (struct_advertised[fd] ~= nil ) then
                        print("\t\t\tfd: " .. "\t" .. tostring(struct_advertised[fd]))
                    end

                    if (struct_advertised[hd] ~= nil ) then
                        print("\t\t\thd: " .. "\t" .. tostring(struct_advertised[hd]))
                    end
                end
            end
        end
    end
end

local function parsingNeighborsVlan(neighbors_vlan, vlan_e)
    print("Vlan: ")

    local vlan_id   = vlan_e.vlan_id
    local pvid      = vlan_e.pvid

    if (neighbors_vlan[vlan_id] ~= nil) then
        print("\t" .. vlan_id .. ":\t" .. tostring(neighbors_vlan[vlan_id]))
    end

    if (neighbors_vlan[vlan_id] ~= nil) then
        print("\t" .. pvid .. ": \t\t" .. tostring(neighbors_vlan[pvid]))
    end
end

local function parsingNeighborsPpvid(neighbors_ppvid, ppvid_e)
    print("PPVID: ")

    local supported   = ppvid_e.supported
    local enabled     = ppvid_e.enabled

    if (neighbors_ppvid[supported] ~= nil) then
        print("\t" .. supported .. ":\t" .. tostring(neighbors_ppvid[supported]))
    end

    if (neighbors_ppvid[enabled] ~= nil) then
        print("\t" .. enabled .. ": \t" .. tostring(neighbors_ppvid[enabled]))
    end
end

local parsingNeighborsFunction = {
    chassis   = utils.parsingChassis,
    port      = parsingNeighborsPort,
    vlan      = parsingNeighborsVlan,
    ppvid     = parsingNeighborsPpvid
}

function lldp_neighbors_module.parsingNeighbors(lldp_neighbors, ports)

    local name = neighbors_e.interface.name
    local via = neighbors_e.interface.via
    local rid = neighbors_e.interface.rid
    local age = neighbors_e.interface.age

    local isPrint = 0
    isPrint = tf.needToPrint(ports, lldp_neighbors[name])
    if isPrint == 1 then
        print("--------------------------------")
        n_portCount = n_portCount + 1
        print(name .. " - " .. lldp_neighbors[name] .. "  |  " ..
            via .. " - " .. lldp_neighbors[via]  .. "  |  " ..
            age .. " - " .. lldp_neighbors[age]  .. "  |  " ..
            rid .. " - " .. lldp_neighbors[rid]  .. "\n")

        local chassis = neighbors_e.interface.chassis
        local port    = neighbors_e.interface.port
        local vlan    = neighbors_e.interface.vlan
        local ppvid   = neighbors_e.interface.ppvid

        if (lldp_neighbors[chassis] ~= nil) then
            for key, value in pairs(lldp_neighbors[chassis]) do
                if (key ~= nil) then
                    if (value == nil) then
                        print ("value is Nill")
                    else
                        print("chassis: ")
                        parsingNeighborsFunction[chassis](value, neighbors_e[chassis])
                    end
                end
            end
        end

        if (lldp_neighbors[port] ~= nil) then
            for  key, value in pairs(lldp_neighbors[port]) do
                if (key ~= nil) then
                    parsingNeighborsFunction[port](value, neighbors_e[port])
                end
            end
        end

        if (lldp_neighbors[vlan] ~= nil) then
            for key, value in pairs(lldp_neighbors[vlan]) do
                if (key ~= nil) then
                    parsingNeighborsFunction[vlan](value, neighbors_e[vlan])
                end
            end
        end

        if (lldp_neighbors[ppvid] ~= nil) then
            for key, value in pairs(lldp_neighbors[ppvid]) do
                if (key ~= nil) then
                    parsingNeighborsFunction[ppvid](value, neighbors_e[ppvid])
                end
            end
        end
    end

    return n_portCount;
end

return lldp_neighbors_module
