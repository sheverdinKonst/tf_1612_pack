'use strict';
'require view';
'require uci';
'require form';



return view.extend({
	render: function() {
		var  m, s, o;

		m = new form.Map('syslog',_('Syslog Settings'));

		s = m.section(form.NamedSection, 'syslog','log',_('Syslog Settings'),_('Syslog Settings description'));

		o = s.option(form.Flag, 'enabled', _('Enable'));
		o.editable = true;

		o = s.option(form.Value, 'log_port', _('Port'));
		o.editable = true;

		o = s.option( form.ListValue, 'log_proto', _('External system log server protocol'))
		o.value('udp', 'UDP')
		o.value('tcp', 'TCP')

		o = s.option(form.DynamicList, 'log_ip', _('Servers'));
		o.datatype = 'host(0)';
		o.ucisection = 'syslog';
		o.load = function(section_id) {
			return uci.get('syslog', 'syslog', 'log_ip');
		};

		return m.render();
	}
});
