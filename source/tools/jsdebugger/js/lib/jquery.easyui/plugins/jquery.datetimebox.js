/**
 * jQuery EasyUI 1.3.2
 * 
 * Copyright (c) 2009-2013 www.jeasyui.com. All rights reserved.
 *
 * Licensed under the GPL or commercial licenses
 * To use it on other terms please contact us: jeasyui@gmail.com
 * http://www.gnu.org/licenses/gpl.txt
 * http://www.jeasyui.com/license_commercial.php
 *
 */
(function($){
function _1(_2){
var _3=$.data(_2,"datetimebox");
var _4=_3.options;
$(_2).datebox($.extend({},_4,{onShowPanel:function(){
var _5=$(_2).datetimebox("getValue");
_9(_2,_5,true);
_4.onShowPanel.call(_2);
},formatter:$.fn.datebox.defaults.formatter,parser:$.fn.datebox.defaults.parser}));
$(_2).removeClass("datebox-f").addClass("datetimebox-f");
$(_2).datebox("calendar").calendar({onSelect:function(_6){
_4.onSelect.call(_2,_6);
}});
var _7=$(_2).datebox("panel");
if(!_3.spinner){
var p=$("<div style=\"padding:2px\"><input style=\"width:80px\"></div>").insertAfter(_7.children("div.datebox-calendar-inner"));
_3.spinner=p.children("input");
var _8=_7.children("div.datebox-button");
var ok=$("<a href=\"javascript:void(0)\" class=\"datebox-ok\"></a>").html(_4.okText).appendTo(_8);
ok.hover(function(){
$(this).addClass("datebox-button-hover");
},function(){
$(this).removeClass("datebox-button-hover");
}).click(function(){
_f(_2);
});
}
_3.spinner.timespinner({showSeconds:_4.showSeconds,separator:_4.timeSeparator}).unbind(".datetimebox").bind("mousedown.datetimebox",function(e){
e.stopPropagation();
});
_9(_2,_4.value);
};
function _a(_b){
var c=$(_b).datetimebox("calendar");
var t=$(_b).datetimebox("spinner");
var _c=c.calendar("options").current;
return new Date(_c.getFullYear(),_c.getMonth(),_c.getDate(),t.timespinner("getHours"),t.timespinner("getMinutes"),t.timespinner("getSeconds"));
};
function _d(_e,q){
_9(_e,q,true);
};
function _f(_10){
var _11=$.data(_10,"datetimebox").options;
var _12=_a(_10);
_9(_10,_11.formatter.call(_10,_12));
$(_10).combo("hidePanel");
};
function _9(_13,_14,_15){
var _16=$.data(_13,"datetimebox").options;
$(_13).combo("setValue",_14);
if(!_15){
if(_14){
var _17=_16.parser.call(_13,_14);
$(_13).combo("setValue",_16.formatter.call(_13,_17));
$(_13).combo("setText",_16.formatter.call(_13,_17));
}else{
$(_13).combo("setText",_14);
}
}
var _17=_16.parser.call(_13,_14);
$(_13).datetimebox("calendar").calendar("moveTo",_17);
$(_13).datetimebox("spinner").timespinner("setValue",_18(_17));
function _18(_19){
function _1a(_1b){
return (_1b<10?"0":"")+_1b;
};
var tt=[_1a(_19.getHours()),_1a(_19.getMinutes())];
if(_16.showSeconds){
tt.push(_1a(_19.getSeconds()));
}
return tt.join($(_13).datetimebox("spinner").timespinner("options").separator);
};
};
$.fn.datetimebox=function(_1c,_1d){
if(typeof _1c=="string"){
var _1e=$.fn.datetimebox.methods[_1c];
if(_1e){
return _1e(this,_1d);
}else{
return this.datebox(_1c,_1d);
}
}
_1c=_1c||{};
return this.each(function(){
var _1f=$.data(this,"datetimebox");
if(_1f){
$.extend(_1f.options,_1c);
}else{
$.data(this,"datetimebox",{options:$.extend({},$.fn.datetimebox.defaults,$.fn.datetimebox.parseOptions(this),_1c)});
}
_1(this);
});
};
$.fn.datetimebox.methods={options:function(jq){
var _20=$.data(jq[0],"datetimebox").options;
_20.originalValue=jq.datebox("options").originalValue;
return _20;
},spinner:function(jq){
return $.data(jq[0],"datetimebox").spinner;
},setValue:function(jq,_21){
return jq.each(function(){
_9(this,_21);
});
},reset:function(jq){
return jq.each(function(){
var _22=$(this).datetimebox("options");
$(this).datetimebox("setValue",_22.originalValue);
});
}};
$.fn.datetimebox.parseOptions=function(_23){
var t=$(_23);
return $.extend({},$.fn.datebox.parseOptions(_23),$.parser.parseOptions(_23,["timeSeparator",{showSeconds:"boolean"}]));
};
$.fn.datetimebox.defaults=$.extend({},$.fn.datebox.defaults,{showSeconds:true,timeSeparator:":",keyHandler:{up:function(){
},down:function(){
},enter:function(){
_f(this);
},query:function(q){
_d(this,q);
}},formatter:function(_24){
var h=_24.getHours();
var M=_24.getMinutes();
var s=_24.getSeconds();
function _25(_26){
return (_26<10?"0":"")+_26;
};
var _27=$(this).datetimebox("spinner").timespinner("options").separator;
var r=$.fn.datebox.defaults.formatter(_24)+" "+_25(h)+_27+_25(M);
if($(this).datetimebox("options").showSeconds){
r+=_27+_25(s);
}
return r;
},parser:function(s){
if($.trim(s)==""){
return new Date();
}
var dt=s.split(" ");
var d=$.fn.datebox.defaults.parser(dt[0]);
if(dt.length<2){
return d;
}
var _28=$(this).datetimebox("spinner").timespinner("options").separator;
var tt=dt[1].split(_28);
var _29=parseInt(tt[0],10)||0;
var _2a=parseInt(tt[1],10)||0;
var _2b=parseInt(tt[2],10)||0;
return new Date(d.getFullYear(),d.getMonth(),d.getDate(),_29,_2a,_2b);
}});
})(jQuery);

