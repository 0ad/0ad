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
function _1(_2,_3){
var _4=$(_2).combo("panel");
var _5=_4.find("div.combobox-item[value=\""+_3+"\"]");
if(_5.length){
if(_5.position().top<=0){
var h=_4.scrollTop()+_5.position().top;
_4.scrollTop(h);
}else{
if(_5.position().top+_5.outerHeight()>_4.height()){
var h=_4.scrollTop()+_5.position().top+_5.outerHeight()-_4.height();
_4.scrollTop(h);
}
}
}
};
function _6(_7){
var _8=$(_7).combo("panel");
var _9=$(_7).combo("getValues");
var _a=_8.find("div.combobox-item[value=\""+_9.pop()+"\"]");
if(_a.length){
var _b=_a.prev(":visible");
if(_b.length){
_a=_b;
}
}else{
_a=_8.find("div.combobox-item:visible:last");
}
var _c=_a.attr("value");
_d(_7,_c);
_1(_7,_c);
};
function _e(_f){
var _10=$(_f).combo("panel");
var _11=$(_f).combo("getValues");
var _12=_10.find("div.combobox-item[value=\""+_11.pop()+"\"]");
if(_12.length){
var _13=_12.next(":visible");
if(_13.length){
_12=_13;
}
}else{
_12=_10.find("div.combobox-item:visible:first");
}
var _14=_12.attr("value");
_d(_f,_14);
_1(_f,_14);
};
function _d(_15,_16){
var _17=$.data(_15,"combobox").options;
var _18=$.data(_15,"combobox").data;
if(_17.multiple){
var _19=$(_15).combo("getValues");
for(var i=0;i<_19.length;i++){
if(_19[i]==_16){
return;
}
}
_19.push(_16);
_1a(_15,_19);
}else{
_1a(_15,[_16]);
}
for(var i=0;i<_18.length;i++){
if(_18[i][_17.valueField]==_16){
_17.onSelect.call(_15,_18[i]);
return;
}
}
};
function _1b(_1c,_1d){
var _1e=$.data(_1c,"combobox").options;
var _1f=$.data(_1c,"combobox").data;
var _20=$(_1c).combo("getValues");
for(var i=0;i<_20.length;i++){
if(_20[i]==_1d){
_20.splice(i,1);
_1a(_1c,_20);
break;
}
}
for(var i=0;i<_1f.length;i++){
if(_1f[i][_1e.valueField]==_1d){
_1e.onUnselect.call(_1c,_1f[i]);
return;
}
}
};
function _1a(_21,_22,_23){
var _24=$.data(_21,"combobox").options;
var _25=$.data(_21,"combobox").data;
var _26=$(_21).combo("panel");
_26.find("div.combobox-item-selected").removeClass("combobox-item-selected");
var vv=[],ss=[];
for(var i=0;i<_22.length;i++){
var v=_22[i];
var s=v;
for(var j=0;j<_25.length;j++){
if(_25[j][_24.valueField]==v){
s=_25[j][_24.textField];
break;
}
}
vv.push(v);
ss.push(s);
_26.find("div.combobox-item[value=\""+v+"\"]").addClass("combobox-item-selected");
}
$(_21).combo("setValues",vv);
if(!_23){
$(_21).combo("setText",ss.join(_24.separator));
}
};
function _27(_28){
var _29=$.data(_28,"combobox").options;
var _2a=[];
$(">option",_28).each(function(){
var _2b={};
_2b[_29.valueField]=$(this).attr("value")!=undefined?$(this).attr("value"):$(this).html();
_2b[_29.textField]=$(this).html();
_2b["selected"]=$(this).attr("selected");
_2a.push(_2b);
});
return _2a;
};
function _2c(_2d,_2e,_2f){
var _30=$.data(_2d,"combobox").options;
var _31=$(_2d).combo("panel");
$.data(_2d,"combobox").data=_2e;
var _32=$(_2d).combobox("getValues");
_31.empty();
for(var i=0;i<_2e.length;i++){
var v=_2e[i][_30.valueField];
var s=_2e[i][_30.textField];
var _33=$("<div class=\"combobox-item\"></div>").appendTo(_31);
_33.attr("value",v);
if(_30.formatter){
_33.html(_30.formatter.call(_2d,_2e[i]));
}else{
_33.html(s);
}
if(_2e[i]["selected"]){
(function(){
for(var i=0;i<_32.length;i++){
if(v==_32[i]){
return;
}
}
_32.push(v);
})();
}
}
if(_30.multiple){
_1a(_2d,_32,_2f);
}else{
if(_32.length){
_1a(_2d,[_32[_32.length-1]],_2f);
}else{
_1a(_2d,[],_2f);
}
}
_30.onLoadSuccess.call(_2d,_2e);
$(".combobox-item",_31).hover(function(){
$(this).addClass("combobox-item-hover");
},function(){
$(this).removeClass("combobox-item-hover");
}).click(function(){
var _34=$(this);
if(_30.multiple){
if(_34.hasClass("combobox-item-selected")){
_1b(_2d,_34.attr("value"));
}else{
_d(_2d,_34.attr("value"));
}
}else{
_d(_2d,_34.attr("value"));
$(_2d).combo("hidePanel");
}
});
};
function _35(_36,url,_37,_38){
var _39=$.data(_36,"combobox").options;
if(url){
_39.url=url;
}
_37=_37||{};
if(_39.onBeforeLoad.call(_36,_37)==false){
return;
}
_39.loader.call(_36,_37,function(_3a){
_2c(_36,_3a,_38);
},function(){
_39.onLoadError.apply(this,arguments);
});
};
function _3b(_3c,q){
var _3d=$.data(_3c,"combobox").options;
if(_3d.multiple&&!q){
_1a(_3c,[],true);
}else{
_1a(_3c,[q],true);
}
if(_3d.mode=="remote"){
_35(_3c,null,{q:q},true);
}else{
var _3e=$(_3c).combo("panel");
_3e.find("div.combobox-item").hide();
var _3f=$.data(_3c,"combobox").data;
for(var i=0;i<_3f.length;i++){
if(_3d.filter.call(_3c,q,_3f[i])){
var v=_3f[i][_3d.valueField];
var s=_3f[i][_3d.textField];
var _40=_3e.find("div.combobox-item[value=\""+v+"\"]");
_40.show();
if(s==q){
_1a(_3c,[v],true);
_40.addClass("combobox-item-selected");
}
}
}
}
};
function _41(_42){
var _43=$.data(_42,"combobox").options;
$(_42).addClass("combobox-f");
$(_42).combo($.extend({},_43,{onShowPanel:function(){
$(_42).combo("panel").find("div.combobox-item").show();
_1(_42,$(_42).combobox("getValue"));
_43.onShowPanel.call(_42);
}}));
};
$.fn.combobox=function(_44,_45){
if(typeof _44=="string"){
var _46=$.fn.combobox.methods[_44];
if(_46){
return _46(this,_45);
}else{
return this.combo(_44,_45);
}
}
_44=_44||{};
return this.each(function(){
var _47=$.data(this,"combobox");
if(_47){
$.extend(_47.options,_44);
_41(this);
}else{
_47=$.data(this,"combobox",{options:$.extend({},$.fn.combobox.defaults,$.fn.combobox.parseOptions(this),_44)});
_41(this);
_2c(this,_27(this));
}
if(_47.options.data){
_2c(this,_47.options.data);
}
_35(this);
});
};
$.fn.combobox.methods={options:function(jq){
var _48=$.data(jq[0],"combobox").options;
_48.originalValue=jq.combo("options").originalValue;
return _48;
},getData:function(jq){
return $.data(jq[0],"combobox").data;
},setValues:function(jq,_49){
return jq.each(function(){
_1a(this,_49);
});
},setValue:function(jq,_4a){
return jq.each(function(){
_1a(this,[_4a]);
});
},clear:function(jq){
return jq.each(function(){
$(this).combo("clear");
var _4b=$(this).combo("panel");
_4b.find("div.combobox-item-selected").removeClass("combobox-item-selected");
});
},reset:function(jq){
return jq.each(function(){
var _4c=$(this).combobox("options");
if(_4c.multiple){
$(this).combobox("setValues",_4c.originalValue);
}else{
$(this).combobox("setValue",_4c.originalValue);
}
});
},loadData:function(jq,_4d){
return jq.each(function(){
_2c(this,_4d);
});
},reload:function(jq,url){
return jq.each(function(){
_35(this,url);
});
},select:function(jq,_4e){
return jq.each(function(){
_d(this,_4e);
});
},unselect:function(jq,_4f){
return jq.each(function(){
_1b(this,_4f);
});
}};
$.fn.combobox.parseOptions=function(_50){
var t=$(_50);
return $.extend({},$.fn.combo.parseOptions(_50),$.parser.parseOptions(_50,["valueField","textField","mode","method","url"]));
};
$.fn.combobox.defaults=$.extend({},$.fn.combo.defaults,{valueField:"value",textField:"text",mode:"local",method:"post",url:null,data:null,keyHandler:{up:function(){
_6(this);
},down:function(){
_e(this);
},enter:function(){
var _51=$(this).combobox("getValues");
$(this).combobox("setValues",_51);
$(this).combobox("hidePanel");
},query:function(q){
_3b(this,q);
}},filter:function(q,row){
var _52=$(this).combobox("options");
return row[_52.textField].indexOf(q)==0;
},formatter:function(row){
var _53=$(this).combobox("options");
return row[_53.textField];
},loader:function(_54,_55,_56){
var _57=$(this).combobox("options");
if(!_57.url){
return false;
}
$.ajax({type:_57.method,url:_57.url,data:_54,dataType:"json",success:function(_58){
_55(_58);
},error:function(){
_56.apply(this,arguments);
}});
},onBeforeLoad:function(_59){
},onLoadSuccess:function(){
},onLoadError:function(){
},onSelect:function(_5a){
},onUnselect:function(_5b){
}});
})(jQuery);

