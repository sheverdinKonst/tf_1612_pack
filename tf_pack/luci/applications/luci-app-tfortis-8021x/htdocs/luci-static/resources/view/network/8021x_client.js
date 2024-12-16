'use strict';
'require form';
'require fs';
'require view';
'require rpc';
'require poll';

function callAutorestartStatus(){
    return fs.exec_direct('/usr/bin/tf_autorestart_ctrl',['status']).then(function(res){
        return res;
    });
}

function callManualReboot(port){
    return fs.exec_direct('/usr/bin/tf_autorestart_ctrl',['reboot',port]).then(function(res){
        return res;
    });
}


var callGetBuiltinEthernetPorts = rpc.declare({
    object: 'luci',
    method: 'getBuiltinEthernetPorts',
    expect: { result: [] }
});

var portlist;


function renderStatus(data) {
    var renderHTML = "";
    var spanTemp = '<span style="color:%s"><strong>%s<br>%s</strong><br>reboots:%s</span>';

    if (data.status == 'OK') {
        renderHTML += String.format(spanTemp, 'green',_("OK"),"",data.reboot_cnt);
    } else if (data.status != 'test disable'){
        renderHTML += String.format(spanTemp, 'red',  _("Fail"),data.status,data.reboot_cnt);
    }

    return renderHTML;
}

function renderStatistics(data) {
    if(data){
        const obj = JSON.parse(data);
        if (typeof  obj !== 'undefined') {
            for (var i = 0; i < portlist.length; i++){
                var view = document.getElementById("status_"+portlist[i].device);
                if(view != null && obj.port[i]!=null)
                    view.innerHTML = renderStatus(obj.port[i]);
            }
        }else{
            console.log("renderStatistics: fail to parse json");
        }
    }
    else
        console.log("renderStatistics: empty string");
}


return L.view.extend({
    /** @private */
    renderPortList: function(data) {
        portlist = data;
    },

    startPolling: function() {
        poll.add(L.bind(function() {
            return L.resolveDefault(callAutorestartStatus()).then(function(data) {
                renderStatistics(data);
            });
        }, this),2);
    },
    load: function() {
        return callGetBuiltinEthernetPorts().then(L.bind(function(data) {
            this.renderPortList(data);
        }, this));
    },


    render: function() {
        var  m, s, o;

        m = new form.Map('cd8021x', _("802.1x Client"),_('Configure IEEE 802.1x wired authentication'));

        s = m.section(form.TypedSection, 'login', _(''));
        s.anonymous = true;
        s.readonly = true;

        o = s.option( form.Flag, 'enable', _('Enable '));
        o.default = o.disabled;
        o.editable = true;

        o = s.option(form.Value, 'username', _('Username'));
        o.editable = true;

        o = s.option(form.Value, 'password', _('Password'));
        o.editable = true;

        o = s.option(form.ListValue, 'ifname', _('Interfaces'));
        for (var i = 0; i < portlist.length; i++){
            o.value(portlist[i].device,getPortName(portlist[i].device));
        }
        o.editable = true;

        o = s.option(form.ListValue, 'eap', _('EAP'));
        o.value("MD5","MD5");
        o.value("PEAP","PEAP");
        o.value("MSCHAPV2","MSCHAPV2");
        o.editable = true;


        return m.render();
    }
});
