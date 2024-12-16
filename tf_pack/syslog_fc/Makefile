include $(TOPDIR)/rules.mk

PKG_NAME:=syslog_fc
PKG_RELEASE:=1

PKG_LICENSE:=GPL-2.0
PKG_MAINTAINER:=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/syslog_fc
  SECTION:=tfortis
  CATEGORY:=TFortis
  TITLE:=syslog_fc
endef

define Package/syslog_fc/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/syslog_fc $(1)/usr/bin/
endef

$(eval $(call BuildPackage,syslog_fc))