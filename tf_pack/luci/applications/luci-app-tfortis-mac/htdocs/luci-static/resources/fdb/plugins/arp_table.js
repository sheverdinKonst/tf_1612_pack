'use strict';

return L.Class.extend({
	title: _('ARP Table'),
	description: 'ARP Table',
	order: 1,
	acl: 'luci-app-tfortis-mac',
	json_data: {
		add_to_downloads: true,
		action: {
			command: '/usr/share/lua/arp_module.lua',
			command_args: [ 'getJson' ]
		}
	},
	columns: [
		{ name: 'ip', display: _('IP') },
		{ name: 'mac', display: _('MAC') },
		{ name: 'device', display: _('Interface') }
	]
});
