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
_3=_3||{};
var _4={};
if(_3.onSubmit){
if(_3.onSubmit.call(_2,_4)==false){
return;
}
}
var _5=$(_2);
if(_3.url){
_5.attr("action",_3.url);
}
var _6="easyui_frame_"+(new Date().getTime());
var _7=$("<iframe id="+_6+" name="+_6+"></iframe>").attr("src",window.ActiveXObject?"javascript:false":"about:blank").css({position:"absolute",top:-1000,left:-1000});
var t=_5.attr("target"),a=_5.attr("action");
_5.attr("target",_6);
var _8=$();
try{
_7.appendTo("body");
_7.bind("load",cb);
for(var n in _4){
var f=$("<input type=\"hidden\" name=\""+n+"\">").val(_4[n]).appendTo(_5);
_8=_8.add(f);
}
_5[0].submit();
}
finally{
_5.attr("action",a);
t?_5.attr("target",t):_5.removeAttr("target");
_8.remove();
}
var _9=10;
function cb(){
_7.unbind();
var _a=$("#"+_6).contents().find("body");
var _b=_a.html();
if(_b==""){
if(--_9){
setTimeout(cb,100);
return;
}
return;
}
var ta=_a.find(">textarea");
if(ta.length){
_b=ta.val();
}else{
var _c=_a.find(">pre");
if(_c.length){
_b=_c.html();
}
}
if(_3.success){
_3.success(_b);
}
setTimeout(function(){
_7.unbind();
_7.remove();
},100);
};
};
function _d(_e,_f){
if(!$.data(_e,"form")){
$.data(_e,"form",{options:$.extend({},$.fn.form.defaults)});
}
var _10=$.data(_e,"form").options;
if(typeof _f=="string"){
var _11={};
if(_10.onBeforeLoad.call(_e,_11)==false){
return;
}
$.ajax({url:_f,data:_11,dataType:"json",success:function(_12){
_13(_12);
},error:function(){
_10.onLoadError.apply(_e,arguments);
}});
}else{
_13(_f);
}
function _13(_14){
var _15=$(_e);
for(var _16 in _14){
var val=_14[_16];
var rr=_17(_16,val);
if(!rr.length){
var f=_15.find("input[numberboxName=\""+_16+"\"]");
if(f.length){
f.numberbox("setValue",val);
}else{
$("input[name=\""+_16+"\"]",_15).val(val);
$("textarea[name=\""+_16+"\"]",_15).val(val);
$("select[name=\""+_16+"\"]",_15).val(val);
}
}
_18(_16,val);
}
_10.onLoadSuccess.call(_e,_14);
_21(_e);
};
function _17(_19,val){
var _1a=$(_e);
var rr=$("input[name=\""+_19+"\"][type=radio], input[name=\""+_19+"\"][type=checkbox]",_1a);
$.fn.prop?rr.prop("checked",false):rr.attr("checked",false);
rr.each(function(){
var f=$(this);
if(f.val()==String(val)){
$.fn.prop?f.prop("checked",true):f.attr("checked",true);
}
});
return rr;
};
function _18(_1b,val){
var _1c=$(_e);
var cc=["combobox","combotree","combogrid","datetimebox","datebox","combo"];
var c=_1c.find("[comboName=\""+_1b+"\"]");
if(c.length){
for(var i=0;i<cc.length;i++){
var _1d=cc[i];
if(c.hasClass(_1d+"-f")){
if(c[_1d]("options").multiple){
c[_1d]("setValues",val);
}else{
c[_1d]("setValue",val);
}
return;
}
}
}
};
};
function _1e(_1f){
$("input,select,textarea",_1f).each(function(){
var t=this.type,tag=this.tagName.toLowerCase();
if(t=="text"||t=="hidden"||t=="password"||tag=="textarea"){
this.value="";
}else{
if(t=="file"){
var _20=$(this);
_20.after(_20.clone().val(""));
_20.remove();
}else{
if(t=="checkbox"||t=="radio"){
this.checked=false;
}else{
if(tag=="select"){
this.selectedIndex=-1;
}
}
}
}
});
if($.fn.combo){
$(".combo-f",_1f).combo("clear");
}
if($.fn.combobox){
$(".combobox-f",_1f).combobox("clear");
}
if($.fn.combotree){
$(".combotree-f",_1f).combotree("clear");
}
if($.fn.combogrid){
$(".combogrid-f",_1f).combogrid("clear");
}
_21(_1f);
};
function _22(_23){
_23.reset();
var t=$(_23);
if($.fn.combo){
t.find(".combo-f").combo("reset");
}
if($.fn.combobox){
t.find(".combobox-f").combobox("reset");
}
if($.fn.combotree){
t.find(".combotree-f").combotree("reset");
}
if($.fn.combogrid){
t.find(".combogrid-f").combogrid("reset");
}
if($.fn.spinner){
t.find(".spinner-f").spinner("reset");
}
if($.fn.timespinner){
t.find(".timespinner-f").timespinner("reset");
}
if($.fn.numberbox){
t.find(".numberbox-f").numberbox("reset");
}
if($.fn.numberspinner){
t.find(".numberspinner-f").numberspinner("reset");
}
_21(_23);
};
function _24(_25){
var _26=$.data(_25,"form").options;
var _27=$(_25);
_27.unbind(".form").bind("submit.form",function(){
setTimeout(function(){
_1(_25,_26);
},0);
return false;
});
};
function _21(_28){
if($.fn.validatebox){
var t=$(_28);
t.find(".validatebox-text:not(:disabled)").validatebox("validate");
var _29=t.find(".validatebox-invalid");
_29.filter(":not(:disabled):first").focus();
return _29.length==0;
}
return true;
};
$.fn.form=function(_2a,_2b){
if(typeof _2a=="string"){
return $.fn.form.methods[_2a](this,_2b);
}
_2a=_2a||{};
return this.each(function(){
if(!$.data(this,"form")){
$.data(this,"form",{options:$.extend({},$.fn.form.defaults,_2a)});
}
_24(this);
});
};
$.fn.form.methods={submit:function(jq,_2c){
return jq.each(function(){
_1(this,$.extend({},$.fn.form.defaults,_2c||{}));
});
},load:function(jq,_2d){
return jq.each(function(){
_d(this,_2d);
});
},clear:function(jq){
return jq.each(function(){
_1e(this);
});
},reset:function(jq){
return jq.each(function(){
_22(this);
});
},validate:function(jq){
return _21(jq[0]);
}};
$.fn.form.defaults={url:null,onSubmit:function(_2e){
return $(this).form("validate");
},success:function(_2f){
},onBeforeLoad:function(_30){
},onLoadSuccess:function(_31){
},onLoadError:function(){
}};
})(jQuery);

