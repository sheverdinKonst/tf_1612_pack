function getDefaultMAC(ifname) {
	console.log(ifname);
	return 'C0:11:A6:00:00:00';
}


function getDefaultSwitch() {
	return 'switch';
}

function open_help_info(){
	window.open('/luci-static/resources/help','Help','width=1000,height=800,toolbar=0,location=0,menubar=0,scrollbars=1,status=0,resizable=0');
	return false;
}



function getPortName(ifname){
	switch (ifname){
		case 'lan1':
			return _('Port 1');
		case 'lan2':
			return _('Port 2');
		case 'lan3':
			return _('Port 3');
		case 'lan4':
			return _('Port 4');
		case 'lan5':
			return _('Port 5');
		case 'lan6':
			return _('Port 6');
		case 'lan7':
			return _('Port 7');
		case 'lan8':
			return _('Port 8');
		case 'lan9':
			return _('Port 9');
		case 'lan10':
			return _('Port 10');
	}
	return ifname;
}
