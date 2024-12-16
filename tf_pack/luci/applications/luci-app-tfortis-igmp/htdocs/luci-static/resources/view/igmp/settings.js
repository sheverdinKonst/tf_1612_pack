'use strict';
'require view';
'require rpc';
'require uci';
'require form';
'require network';
'require firewall';
'require tools.firewall as fwtool';
'require tools.widgets as widgets';


return view.extend({
	render: function() {
		var  m, s, o;
		
		
		m = new form.Map('network');

		m.tabbed =  false;

		
		s = m.section(form.NamedSection, 'switch', 'device', _('IGMP Settings'),
			_('This page allows you to see ..................'));

		o = s.option( form.Flag, 'igmp_snooping', _('Enable <abbr title="Internet Group Management Protocol">IGMP</abbr> snooping'), _('Enables IGMP snooping on this bridge'));
		o.default = o.disabled;
		o.editable = true;
		//o.depends('type', 'bridge');
		
		o = s.option(form.Value, 'hash_max', _('Maximum snooping table size'));
		o.placeholder = '512';
		o.editable = true;
		o.datatype = 'uinteger';
		//o.depends({ type: 'bridge', igmp_snooping: '1' });

		o = s.option(form.Flag, 'multicast_querier', _('Enable multicast querier'));
		o.defaults = { '1': [{'igmp_snooping': '1'}], '0': [{'igmp_snooping': '0'}] };
		//o.depends('type', 'bridge');

		o = s.option( form.Value, 'robustness', _('Robustness'), _('The robustness value allows tuning for the expected packet loss on the network. If a network is expected to be lossy, the robustness value may be increased. IGMP is robust to (Robustness-1) packet losses'));
		o.placeholder = '2';
		o.datatype = 'min(1)';
		//o.depends({ type: 'bridge', multicast_querier: '1' });

		o = s.option(form.Value, 'query_interval', _('Query interval'), _('Interval in centiseconds between multicast general queries. By varying the value, an administrator may tune the number of IGMP messages on the subnet; larger values cause IGMP Queries to be sent less often'));
		o.placeholder = '12500';
		o.datatype = 'uinteger';
		//o.depends({ type: 'bridge', multicast_querier: '1' });

		o = s.option( form.Value, 'query_response_interval', _('Query response interval'), _('The max response time in centiseconds inserted into the periodic general queries. By varying the value, an administrator may tune the burstiness of IGMP messages on the subnet; larger values make the traffic less bursty, as host responses are spread out over a larger interval'));
		o.placeholder = '1000';
		o.datatype = 'uinteger';
		o.validate = function(section_id, value) {
			var qiopt = L.toArray(this.map.lookupOption('query_interval', section_id))[0],
			    qival = qiopt ? (qiopt.formvalue(section_id) || qiopt.placeholder) : '';

			if (value != '' && qival != '' && +value >= +qival)
				return _('The query response interval must be lower than the query interval value');

			return true;
		};
		//o.depends({ type: 'bridge', multicast_querier: '1' });

		o = s.option( form.Value, 'last_member_interval', _('Last member interval'), _('The max response time in centiseconds inserted into group-specific queries sent in response to leave group messages. It is also the amount of time between group-specific query messages. This value may be tuned to modify the "leave latency" of the network. A reduced value results in reduced time to detect the loss of the last member of a group'));
		o.placeholder = '100';
		o.datatype = 'uinteger';
		//o.depends({ type: 'bridge', multicast_querier: '1' });

		o = s.option(form.ListValue, 'igmpversion', _('Force IGMP version'));
		o.value('', _('No enforcement'));
		o.value('1', _('Enforce IGMPv1'));
		o.value('2', _('Enforce IGMPv2'));
		o.value('3', _('Enforce IGMPv3'));
		//o.depends('multicast', /1/);

		o = s.option(form.ListValue, 'mldversion', _('Force MLD version'));
		o.value('', _('No enforcement'));
		o.value('1', _('Enforce MLD version 1'));
		o.value('2', _('Enforce MLD version 2'));
		//o.depends('multicast', /1/);

		/*if (isBridgePort(dev)) {
			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'learning', _('Enable MAC address learning'));
			o.sysfs = '/sys/class/net/%s/brport/learning'.format(devname || 'default');

			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'unicast_flood', _('Enable unicast flooding'));
			o.sysfs = '/sys/class/net/%s/brport/unicast_flood'.format(devname || 'default');

			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'isolate', _('Port isolation'), _('Only allow communication with non-isolated bridge ports when enabled'));
			o.sysfs = '/sys/class/net/%s/brport/isolated'.format(devname || 'default');

			o = this.replaceOption(s, 'brport', form.ListValue, 'multicast_router', _('Multicast routing'));
			o.value('', _('Never'));
			o.value('1', _('Learn'));
			o.value('2', _('Always'));
			o.depends('multicast', /1/);

			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'multicast_to_unicast', _('Multicast to unicast'), _('Forward multicast packets as unicast packets on this device.'));
			o.sysfs = '/sys/class/net/%s/brport/multicast_to_unicast'.format(devname || 'default');
			o.depends('multicast', /1/);

			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'multicast_fast_leave', _('Enable multicast fast leave'));
			o.sysfs = '/sys/class/net/%s/brport/multicast_fast_leave'.format(devname || 'default');
			o.depends('multicast', /1/);

			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'drop_v4_unicast_in_l2_multicast', _('Drop nested IPv4 unicast'), _('Drop layer 2 multicast frames containing IPv4 unicast packets.'));
			o.sysfs = '/proc/sys/net/ipv4/conf/%s/drop_unicast_in_l2_multicast'.format(devname || 'default');
			o.depends('multicast', /1/);

			o = this.replaceOption(s, 'brport', cbiFlagTristate, 'drop_v6_unicast_in_l2_multicast', _('Drop nested IPv6 unicast'), _('Drop layer 2 multicast frames containing IPv6 unicast packets.'));
			o.sysfs = '/proc/sys/net/ipv6/conf/%s/drop_unicast_in_l2_multicast'.format(devname || 'default');
			o.depends('multicast', /1/);
		}*/
		
		
		
		
	
		return m.render();
	}
});
