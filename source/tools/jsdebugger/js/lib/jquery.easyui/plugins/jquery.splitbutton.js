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
var _3=$.data(_2,"splitbutton").options;
var _4=$(_2);
_4.removeClass("s-btn-active s-btn-plain-active").addClass("s-btn");
_4.linkbutton($.extend({},_3,{text:_3.text+"<span class=\"s-btn-downarrow\">&nbsp;</span>"}));
if(_3.menu){
$(_3.menu).menu({onShow:function(){
_4.addClass((_3.plain==true)?"s-btn-plain-active":"s-btn-active");
},onHide:function(){
_4.removeClass((_3.plain==true)?"s-btn-plain-active":"s-btn-active");
}});
}
_5(_2,_3.disabled);
};
function _5(_6,_7){
var _8=$.data(_6,"splitbutton").options;
_8.disabled=_7;
var _9=$(_6);
var _a=_9.find(".s-btn-downarrow");
if(_7){
_9.linkbutton("disable");
_a.unbind(".splitbutton");
}else{
_9.linkbutton("enable");
_a.unbind(".splitbutton");
_a.bind("click.splitbutton",function(){
_b();
return false;
});
var _c=null;
_a.bind("mouseenter.splitbutton",function(){
_c=setTimeout(function(){
_b();
},_8.duration);
return false;
}).bind("mouseleave.splitbutton",function(){
if(_c){
clearTimeout(_c);
}
});
}
function _b(){
if(!_8.menu){
return;
}
$("body>div.menu-top").menu("hide");
$(_8.menu).menu("show",{alignTo:_9});
_9.blur();
};
};
$.fn.splitbutton=function(_d,_e){
if(typeof _d=="string"){
return $.fn.splitbutton.methods[_d](this,_e);
}
_d=_d||{};
return this.each(function(){
var _f=$.data(this,"splitbutton");
if(_f){
$.extend(_f.options,_d);
}else{
$.data(this,"splitbutton",{options:$.extend({},$.fn.splitbutton.defaults,$.fn.splitbutton.parseOptions(this),_d)});
$(this).removeAttr("disabled");
}
_1(this);
});
};
$.fn.splitbutton.methods={options:function(jq){
return $.data(jq[0],"splitbutton").options;
},enable:function(jq){
return jq.each(function(){
_5(this,false);
});
},disable:function(jq){
return jq.each(function(){
_5(this,true);
});
},destroy:function(jq){
return jq.each(function(){
var _10=$(this).splitbutton("options");
if(_10.menu){
$(_10.menu).menu("destroy");
}
$(this).remove();
});
}};
$.fn.splitbutton.parseOptions=function(_11){
var t=$(_11);
return $.extend({},$.fn.linkbutton.parseOptions(_11),$.parser.parseOptions(_11,["menu",{plain:"boolean",duration:"number"}]));
};
$.fn.splitbutton.defaults=$.extend({},$.fn.linkbutton.defaults,{plain:true,menu:null,duration:100});
})(jQuery);

