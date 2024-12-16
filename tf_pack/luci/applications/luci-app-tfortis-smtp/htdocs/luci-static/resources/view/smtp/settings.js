'use strict';
'require view';
'require rpc';
'require uci';
'require form';
'require fs';

var testResult;

function renderReply(res){

    testResult = res;
    renderStatistics(res);
}
function renderStatistics(data) {
    if(data){
        var view = document.getElementById("send_result");
        if(view != null){
            var spanTemp = '<span style="color:%s"><strong>%s</strong>';
            if(data.code == 0)
                view.innerHTML = String.format(spanTemp, 'green',_("OK"));
            else
                view.innerHTML = String.format(spanTemp, 'red',  data.stderr);
        }
    }
    else
        console.log("renderStatistics: empty string");
}


function callTestEmail(subject,to){

    console.log("subject" + subject);
    console.log("to" + to);


    return fs.exec("/usr/libexec/network/sendSmtp.sh",[subject,'TFortis test email',to]).then(function(res){
        renderReply(res);
    });
}



return view.extend({
    render: function() {
        var  m, s, o;

        m = new form.Map('smtp');

        //m.tabbed =  false;
        s = m.section(form.NamedSection, 'client',  _('SMTP Client Settings'),  _('SMTP Client Settings'));


        o = s.option( form.Flag, 'enabled', _('Enable '));
        o.default = o.disabled;
        o.editable = true;

        o = s.option(form.Value, 'host', _('SMTP Host'));
        o.editable = true;
        o.datatype  = 'host';


        o = s.option(form.Value, 'user', _('User'));
        o.editable = true;

        o = s.option(form.Value, 'password', _('Password'));
        o.editable = true;
        o.password = true;

        o = s.option(form.Value, 'port', _('Port'));
        o.placeholder = '25';
        o.editable = true;
        o.datatype = 'uinteger';

        o = s.option(form.Flag, 'tls', _('Use TLS'));
        o.defaults = '0';


        o = s.option(form.Value, 'subject', _('Mail Subject'));
        o.editable = true;


        o = s.option(form.Value, 'to1', _('Receiver 1'));
        o.editable = true;
        o.datatype = 'email';

        o = s.option(form.Value, 'to2', _('Receiver 2'));
        o.editable = true;
        o.datatype = 'email';

        o = s.option(form.Value, 'to3', _('Receiver 2'));
        o.editable = true;
        o.datatype = 'email';



        o = s.option(form.Button, 'test', _('Send Test Email'));
        o.inputstyle = 'action important';
        o.inputtitle = _('Test');
        o.editable = true;
        o.modalonly = false;
        o.onclick = L.bind(function(ev, section_id) {
            var to,to1,to2,to3;
            to1=uci.get('smtp', section_id, 'to1');
            if(typeof to1 !== 'undefined')
                to = to1
            to2=uci.get('smtp', section_id, 'to2');
            if(typeof to2 !== 'undefined')
                to += (',' + to2)
            to2=uci.get('smtp', section_id, 'to2');
            if(typeof to3 !== 'undefined')
                to += (',' + to3)

            return callTestEmail(uci.get('smtp', section_id, 'subject'),to);
        }, this);

        o = s.option(form.DummyValue, 'result', _('Test result'));
        o.editable = false;
        o.modalonly = false;
        o.cfgvalue = function(section_id) {
            return E('span', { 'id': 'send_result' }, [ _('') ])
        }

        return m.render();
    }
});
