#
# Copyright (C) 2024 Fort Telecom
#
# This is free software, licensed under the Apache License, Version 2.0 .
#

include $(TOPDIR)/rules.mk

LUCI_TITLE:=TFortis Theme 
LUCI_DEPENDS:=

define Package/luci-theme-tfortis/postrm
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || {
	uci -q delete luci.themes.TFortis
	uci commit luci
}
endef

#include $(TOPDIR)/feeds/luci/luci.mk
include ../../luci.mk

# call BuildPackage - OpenWrt buildroot signature
