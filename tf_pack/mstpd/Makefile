#
# Copyright (C) 2019 Alexandru Ardelean <ardeleanalex@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mstpd
PKG_VERSION:=1.0.0
PKG_RELEASE:=3

PKG_MAINTAINER:=Alexandru Ardelean <ardeleanalex@gmail.com>
PKG_LICENSE:=GPL-2.0-or-later
PKG_LICENSE_FILES:=LICENSE

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/mstpd
  SECTION:=tfortis
  CATEGORY:=TFortis
  DEFAULT:=y
  TITLE:=Multiple Spanning Tree Protocol daemon
  URL:=https://github.com/mstpd/mstpd
endef

define Package/mstpd/description
  Multiple Spanning Tree Protocol daemon.
  Implements MSTP which is not implemented yet in the Linux kernel.
endef

define Package/mstpd/config
  source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += \
	-DRTNL_RCV_BUFSIZE=$(CONFIG_MSTPD_RTNL_RCV_BUFSIZE) \
	-DRTNL_SND_BUFSIZE=$(CONFIG_MSTPD_RTNL_SND_BUFSIZE)

MAKE_VARS+=MODE=prod

define Package/mstpd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mstpd $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mstpctl $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/etc/init.d/mstpd $(1)/etc/init.d/mstpd
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) ./files/sbin/bridge-stp $(1)/sbin
	$(INSTALL_DIR) $(1)/lib/functions
	$(INSTALL_BIN) ./files/lib/functions/mstpd.sh $(1)/lib/functions
	$(INSTALL_DIR) $(1)/usr/libexec/mstpctl-utils
	$(INSTALL_BIN) ./files/usr/libexec/mstpctl-utils/mstpd-config-migrate.sh $(1)/usr/libexec/mstpctl-utils
	$(INSTALL_DIR) $(1)/usr/libexec/rpcd
	$(INSTALL_BIN) ./files/usr/libexec/rpcd/mstpd $(1)/usr/libexec/rpcd
endef

$(eval $(call BuildPackage,mstpd))
