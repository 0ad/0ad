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
$(_2).addClass("validatebox-text");
};
function _3(_4){
var _5=$.data(_4,"validatebox");
_5.validating=false;
var _6=_5.tip;
if(_6){
_6.remove();
}
$(_4).unbind();
$(_4).remove();
};
function _7(_8){
var _9=$(_8);
var _a=$.data(_8,"validatebox");
_9.unbind(".validatebox").bind("focus.validatebox",function(){
_a.validating=true;
_a.value=undefined;
(function(){
if(_a.validating){
if(_a.value!=_9.val()){
_a.value=_9.val();
if(_a.timer){
clearTimeout(_a.timer);
}
_a.timer=setTimeout(function(){
$(_8).validatebox("validate");
},_a.options.delay);
}else{
_10(_8);
}
setTimeout(arguments.callee,200);
}
})();
}).bind("blur.validatebox",function(){
if(_a.timer){
clearTimeout(_a.timer);
_a.timer=undefined;
}
_a.validating=false;
_b(_8);
}).bind("mouseenter.validatebox",function(){
if(_9.hasClass("validatebox-invalid")){
_c(_8);
}
}).bind("mouseleave.validatebox",function(){
if(!_a.validating){
_b(_8);
}
});
};
function _c(_d){
var _e=$.data(_d,"validatebox").message;
var _f=$.data(_d,"validatebox").tip;
if(!_f){
_f=$("<div class=\"validatebox-tip\">"+"<span class=\"validatebox-tip-content\">"+"</span>"+"<span class=\"validatebox-tip-pointer\">"+"</span>"+"</div>").appendTo("body");
$.data(_d,"validatebox").tip=_f;
}
_f.find(".validatebox-tip-content").html(_e);
_10(_d);
};
function _10(_11){
var _12=$.data(_11,"validatebox");
if(!_12){
return;
}
var tip=_12.tip;
if(tip){
var box=$(_11);
var _13=tip.find(".validatebox-tip-pointer");
var _14=tip.find(".validatebox-tip-content");
tip.show();
tip.css("top",box.offset().top-(_14._outerHeight()-box._outerHeight())/2);
if(_12.options.tipPosition=="left"){
tip.css("left",box.offset().left-tip._outerWidth());
tip.addClass("validatebox-tip-left");
}else{
tip.css("left",box.offset().left+box._outerWidth());
tip.removeClass("validatebox-tip-left");
}
_13.css("top",(_14._outerHeight()-_13._outerHeight())/2);
}
};
function _b(_15){
var tip=$.data(_15,"validatebox").tip;
if(tip){
tip.remove();
$.data(_15,"validatebox").tip=null;
}
};
function _16(_17){
var _18=$.data(_17,"validatebox");
var _19=_18.options;
var tip=_18.tip;
var box=$(_17);
var _1a=box.val();
function _1b(msg){
_18.message=msg;
};
function _1c(_1d){
var _1e=/([a-zA-Z_]+)(.*)/.exec(_1d);
var _1f=_19.rules[_1e[1]];
if(_1f&&_1a){
var _20=eval(_1e[2]);
if(!_1f["validator"](_1a,_20)){
box.addClass("validatebox-invalid");
var _21=_1f["message"];
if(_20){
for(var i=0;i<_20.length;i++){
_21=_21.replace(new RegExp("\\{"+i+"\\}","g"),_20[i]);
}
}
_1b(_19.invalidMessage||_21);
if(_18.validating){
_c(_17);
}
return false;
}
}
return true;
};
if(_19.required){
if(_1a==""){
box.addClass("validatebox-invalid");
_1b(_19.missingMessage);
if(_18.validating){
_c(_17);
}
return false;
}
}
if(_19.validType){
if(typeof _19.validType=="string"){
if(!_1c(_19.validType)){
return false;
}
}else{
for(var i=0;i<_19.validType.length;i++){
if(!_1c(_19.validType[i])){
return false;
}
}
}
}
box.removeClass("validatebox-invalid");
_b(_17);
return true;
};
$.fn.validatebox=function(_22,_23){
if(typeof _22=="string"){
return $.fn.validatebox.methods[_22](this,_23);
}
_22=_22||{};
return this.each(function(){
var _24=$.data(this,"validatebox");
if(_24){
$.extend(_24.options,_22);
}else{
_1(this);
$.data(this,"validatebox",{options:$.extend({},$.fn.validatebox.defaults,$.fn.validatebox.parseOptions(this),_22)});
}
_7(this);
});
};
$.fn.validatebox.methods={destroy:function(jq){
return jq.each(function(){
_3(this);
});
},validate:function(jq){
return jq.each(function(){
_16(this);
});
},isValid:function(jq){
return _16(jq[0]);
}};
$.fn.validatebox.parseOptions=function(_25){
var t=$(_25);
return $.extend({},$.parser.parseOptions(_25,["validType","missingMessage","invalidMessage","tipPosition",{delay:"number"}]),{required:(t.attr("required")?true:undefined)});
};
$.fn.validatebox.defaults={required:false,validType:null,delay:200,missingMessage:"This field is required.",invalidMessage:null,tipPosition:"right",rules:{email:{validator:function(_26){
return /^((([a-z]|\d|[!#\$%&'\*\+\-\/=\?\^_`{\|}~]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])+(\.([a-z]|\d|[!#\$%&'\*\+\-\/=\?\^_`{\|}~]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])+)*)|((\x22)((((\x20|\x09)*(\x0d\x0a))?(\x20|\x09)+)?(([\x01-\x08\x0b\x0c\x0e-\x1f\x7f]|\x21|[\x23-\x5b]|[\x5d-\x7e]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(\\([\x01-\x09\x0b\x0c\x0d-\x7f]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF]))))*(((\x20|\x09)*(\x0d\x0a))?(\x20|\x09)+)?(\x22)))@((([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.)+(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.?$/i.test(_26);
},message:"Please enter a valid email address."},url:{validator:function(_27){
return /^(https?|ftp):\/\/(((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:)*@)?(((\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5])\.(\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5])\.(\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5])\.(\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5]))|((([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|\d|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.)+(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])*([a-z]|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])))\.?)(:\d*)?)(\/((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)+(\/(([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)*)*)?)?(\?((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)|[\uE000-\uF8FF]|\/|\?)*)?(\#((([a-z]|\d|-|\.|_|~|[\u00A0-\uD7FF\uF900-\uFDCF\uFDF0-\uFFEF])|(%[\da-f]{2})|[!\$&'\(\)\*\+,;=]|:|@)|\/|\?)*)?$/i.test(_27);
},message:"Please enter a valid URL."},length:{validator:function(_28,_29){
var len=$.trim(_28).length;
return len>=_29[0]&&len<=_29[1];
},message:"Please enter a value between {0} and {1}."},remote:{validator:function(_2a,_2b){
var _2c={};
_2c[_2b[1]]=_2a;
var _2d=$.ajax({url:_2b[0],dataType:"json",data:_2c,async:false,cache:false,type:"post"}).responseText;
return _2d=="true";
},message:"Please fix this field."}}};
})(jQuery);

