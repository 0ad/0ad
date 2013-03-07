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
var _3=$.data(_2,"linkbutton").options;
$(_2).empty();
$(_2).addClass("l-btn");
if(_3.id){
$(_2).attr("id",_3.id);
}else{
$(_2).attr("id","");
}
if(_3.plain){
$(_2).addClass("l-btn-plain");
}else{
$(_2).removeClass("l-btn-plain");
}
if(_3.text){
$(_2).html(_3.text).wrapInner("<span class=\"l-btn-left\">"+"<span class=\"l-btn-text\">"+"</span>"+"</span>");
if(_3.iconCls){
$(_2).find(".l-btn-text").addClass(_3.iconCls).addClass(_3.iconAlign=="left"?"l-btn-icon-left":"l-btn-icon-right");
}
}else{
$(_2).html("&nbsp;").wrapInner("<span class=\"l-btn-left\">"+"<span class=\"l-btn-text\">"+"<span class=\"l-btn-empty\"></span>"+"</span>"+"</span>");
if(_3.iconCls){
$(_2).find(".l-btn-empty").addClass(_3.iconCls);
}
}
$(_2).unbind(".linkbutton").bind("focus.linkbutton",function(){
if(!_3.disabled){
$(this).find("span.l-btn-text").addClass("l-btn-focus");
}
}).bind("blur.linkbutton",function(){
$(this).find("span.l-btn-text").removeClass("l-btn-focus");
});
_4(_2,_3.disabled);
};
function _4(_5,_6){
var _7=$.data(_5,"linkbutton");
if(_6){
_7.options.disabled=true;
var _8=$(_5).attr("href");
if(_8){
_7.href=_8;
$(_5).attr("href","javascript:void(0)");
}
if(_5.onclick){
_7.onclick=_5.onclick;
_5.onclick=null;
}
$(_5).addClass("l-btn-disabled");
}else{
_7.options.disabled=false;
if(_7.href){
$(_5).attr("href",_7.href);
}
if(_7.onclick){
_5.onclick=_7.onclick;
}
$(_5).removeClass("l-btn-disabled");
}
};
$.fn.linkbutton=function(_9,_a){
if(typeof _9=="string"){
return $.fn.linkbutton.methods[_9](this,_a);
}
_9=_9||{};
return this.each(function(){
var _b=$.data(this,"linkbutton");
if(_b){
$.extend(_b.options,_9);
}else{
$.data(this,"linkbutton",{options:$.extend({},$.fn.linkbutton.defaults,$.fn.linkbutton.parseOptions(this),_9)});
$(this).removeAttr("disabled");
}
_1(this);
});
};
$.fn.linkbutton.methods={options:function(jq){
return $.data(jq[0],"linkbutton").options;
},enable:function(jq){
return jq.each(function(){
_4(this,false);
});
},disable:function(jq){
return jq.each(function(){
_4(this,true);
});
}};
$.fn.linkbutton.parseOptions=function(_c){
var t=$(_c);
return $.extend({},$.parser.parseOptions(_c,["id","iconCls","iconAlign",{plain:"boolean"}]),{disabled:(t.attr("disabled")?true:undefined),text:$.trim(t.html()),iconCls:(t.attr("icon")||t.attr("iconCls"))});
};
$.fn.linkbutton.defaults={id:null,disabled:false,plain:false,text:"",iconCls:null,iconAlign:"left"};
})(jQuery);

