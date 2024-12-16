#
# This software is licensed under the Public Domain.21
#
include $(TOPDIR)/rules.mk

PKG_NAME:=tf_testPcap

#PKG_VERSION:= $(shell sed -n 's/^#define PKG_VERSION "\(.*\)"/\1/p' src/hwsys_ctrl/hwsys_ctrl.h)
PKG_VERSION:= 0.0
#PKG_RELEASE:= $(shellsed -n 's/^#define PKG_RELEASE "\(.*\)"/\1/p' src/hwsys_ctrl/hwsys_ctrl.h)
PKG_RELEASE:= 1

PKG_MAINTAINER:=Konstantin Shevedrin <sheverdin@fort-telecom.ru>
PKG_LICENSE:=CC0-1.0

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/tf_testPcap
	SECTION:=tfortis
	# Select package by default
	DEFAULT:=y
	CATEGORY:=TFortis
	DEPENDS:=+libpcap
	TITLE:=	tf_testPcap
	URL:=https://www.tfortis.com
endef

define Package/tf_testPcap/description
	test app for anylize vlan trafic
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/tf_testPcap/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tf_testPcap $(1)/usr/bin/

endef

$(eval $(call BuildPackage,tf_testPcap))
