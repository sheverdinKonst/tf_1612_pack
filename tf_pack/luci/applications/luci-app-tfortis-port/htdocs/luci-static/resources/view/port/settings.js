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

		m = new form.Map('port');

		s = m.section(form.GridSection, 'lan',  _('Port Settings'),	_('Configuration of port settings: state, speed, duplex, flow control, PoE'));
		s.anonymous = true;
		//s.sortable  = true;
		//s.nodescriptions = false;

		s.tab('settings', _('Settings'));

		o = s.taboption('settings',form.DummyValue, 'ifn', 'Interface');
		o.cfgvalue = function(section_id) {
			return getPortName(section_id);
		}

		o = s.taboption('settings',form.ListValue, 'state', 'State');
		o.value('enable','enable');
		o.value('disable','disable');	
		//o.rmempty = false;
		o.editable = true;
		
		o = s.taboption('settings',form.ListValue, 'speed', _('Speed/Duplex'));
		o.value('10 duplex full', _('10Full'));
		o.value('10 duplex half', _('10Half'));
		o.value('100 duplex full', _('100Full'));
		o.value('100 duplex half', _('100Half'));
		o.value('1000 duplex full', _('1000Full'));
		o.value('auto', _('Auto'));
		o.editable = true;
		
		o = s.taboption('settings',form.ListValue, 'flow', _('Flow Control'));
		o.value('enable', _('enable'));
		o.value('disable', _('disable'));	
		o.editable = true;
		
		o = s.taboption('settings',form.ListValue, 'poe', _('PoE'));
		o.value('enable', _('enable'));
		o.value('disable', _('disable'));	
		o.editable = true;
	
		return m.render();
	}
});
