'use strict';

return L.Class.extend({
	title: _('MAC Table'),
	description: 'MAC Table',
	order: 0,
	acl: 'luci-app-tfortis-mac',
	json_data: {
		add_to_downloads: true,
		action: {
			command: '/usr/share/lua/fdb_module.lua',
			command_args: [ 'getJson' ]
		}
	},
	columns: [
		{ name: 'mac', display: _('MAC') },
		{ name: 'vid', display: _('VLAN') },
		{ name: 'interface', display: _('Interface') },
		{ name: 'age', display: _('Age') }
	]
});
