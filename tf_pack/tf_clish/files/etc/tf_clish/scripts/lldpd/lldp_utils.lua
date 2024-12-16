
local lldp_utils = {}

lldp_utils.id_e =
{
    type    = "type",
    value   = "value",
}

lldp_utils.capability_e =
{
    type    = "type",
    enabled   = "enabled",
}

lldp_utils.chassis_e = {
    id              = "id",
    name            = "name",
    descr           = "descr",
    mgmt_ip         = "mgmt-ip",
    mgmt_iface      = "mgmt-iface",
    capability      = "capability",
    id_t            = lldp_utils.id_e,
    capability_t    = lldp_utils.capability_e
}

lldp_utils.configList =
{

    { name = "enable_lldp",      str = "\tenabled:     " },
    { name = "force_lldp",       str = "\tforce LLDP:  " },
    { name = "enable_cdp",       str = "\tCDP:         " },
    { name = "enable_fdp",       str = "\tFDP:         " },
    { name = "enable_sonmp",     str = "\tSONMP:       " },
    { name = "enable_edp",       str = "\tEDP:         " },
    { name = "lldp_tx_interval", str = "\tTX interval: " },
    { name = "lldp_tx_hold",     str = "\tTX hold:     " },
    { name = "interface",        srt = "Interface:     " }
}

return lldp_utils





