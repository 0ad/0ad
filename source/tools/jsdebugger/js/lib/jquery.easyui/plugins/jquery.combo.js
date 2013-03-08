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
var _4=$.data(_2,"combo").options;
var _5=$.data(_2,"combo").combo;
var _6=$.data(_2,"combo").panel;
if(_3){
_4.width=_3;
}
if(isNaN(_4.width)){
var c=$(_2).clone();
c.css("visibility","hidden");
c.appendTo("body");
_4.width=c.outerWidth();
c.remove();
}
_5.appendTo("body");
var _7=_5.find("input.combo-text");
var _8=_5.find(".combo-arrow");
var _9=_4.hasDownArrow?_8._outerWidth():0;
_5._outerWidth(_4.width)._outerHeight(_4.height);
_7._outerWidth(_5.width()-_9);
_7.css({height:_5.height()+"px",lineHeight:_5.height()+"px"});
_8._outerHeight(_5.height());
_6.panel("resize",{width:(_4.panelWidth?_4.panelWidth:_5.outerWidth()),height:_4.panelHeight});
_5.insertAfter(_2);
};
function _a(_b){
var _c=$.data(_b,"combo").options;
var _d=$.data(_b,"combo").combo;
if(_c.hasDownArrow){
_d.find(".combo-arrow").show();
}else{
_d.find(".combo-arrow").hide();
}
};
function _e(_f){
$(_f).addClass("combo-f").hide();
var _10=$("<span class=\"combo\"></span>").insertAfter(_f);
var _11=$("<input type=\"text\" class=\"combo-text\">").appendTo(_10);
$("<span><span class=\"combo-arrow\"></span></span>").appendTo(_10);
$("<input type=\"hidden\" class=\"combo-value\">").appendTo(_10);
var _12=$("<div class=\"combo-panel\"></div>").appendTo("body");
_12.panel({doSize:false,closed:true,cls:"combo-p",style:{position:"absolute",zIndex:10},onOpen:function(){
$(this).panel("resize");
}});
var _13=$(_f).attr("name");
if(_13){
_10.find("input.combo-value").attr("name",_13);
$(_f).removeAttr("name").attr("comboName",_13);
}
_11.attr("autocomplete","off");
return {combo:_10,panel:_12};
};
function _14(_15){
var _16=$.data(_15,"combo").combo.find("input.combo-text");
_16.validatebox("destroy");
$.data(_15,"combo").panel.panel("destroy");
$.data(_15,"combo").combo.remove();
$(_15).remove();
};
function _17(_18){
var _19=$.data(_18,"combo");
var _1a=_19.options;
var _1b=$.data(_18,"combo").combo;
var _1c=$.data(_18,"combo").panel;
var _1d=_1b.find(".combo-text");
var _1e=_1b.find(".combo-arrow");
$(document).unbind(".combo").bind("mousedown.combo",function(e){
var p=$(e.target).closest("span.combo,div.combo-panel");
if(p.length){
return;
}
var _1f=$("body>div.combo-p>div.combo-panel");
_1f.panel("close");
});
_1b.unbind(".combo");
_1c.unbind(".combo");
_1d.unbind(".combo");
_1e.unbind(".combo");
if(!_1a.disabled){
_1d.bind("mousedown.combo",function(e){
$("div.combo-panel").not(_1c).panel("close");
e.stopPropagation();
}).bind("keydown.combo",function(e){
switch(e.keyCode){
case 38:
_1a.keyHandler.up.call(_18);
break;
case 40:
_1a.keyHandler.down.call(_18);
break;
case 13:
e.preventDefault();
_1a.keyHandler.enter.call(_18);
return false;
case 9:
case 27:
_28(_18);
break;
default:
if(_1a.editable){
if(_19.timer){
clearTimeout(_19.timer);
}
_19.timer=setTimeout(function(){
var q=_1d.val();
if(_19.previousValue!=q){
_19.previousValue=q;
$(_18).combo("showPanel");
_1a.keyHandler.query.call(_18,_1d.val());
_2c(_18,true);
}
},_1a.delay);
}
}
});
_1e.bind("click.combo",function(){
if(_1c.is(":visible")){
_28(_18);
}else{
$("div.combo-panel").panel("close");
$(_18).combo("showPanel");
}
_1d.focus();
}).bind("mouseenter.combo",function(){
$(this).addClass("combo-arrow-hover");
}).bind("mouseleave.combo",function(){
$(this).removeClass("combo-arrow-hover");
}).bind("mousedown.combo",function(){
});
}
};
function _20(_21){
var _22=$.data(_21,"combo").options;
var _23=$.data(_21,"combo").combo;
var _24=$.data(_21,"combo").panel;
if($.fn.window){
_24.panel("panel").css("z-index",$.fn.window.defaults.zIndex++);
}
_24.panel("move",{left:_23.offset().left,top:_25()});
if(_24.panel("options").closed){
_24.panel("open");
_22.onShowPanel.call(_21);
}
(function(){
if(_24.is(":visible")){
_24.panel("move",{left:_26(),top:_25()});
setTimeout(arguments.callee,200);
}
})();
function _26(){
var _27=_23.offset().left;
if(_27+_24._outerWidth()>$(window)._outerWidth()+$(document).scrollLeft()){
_27=$(window)._outerWidth()+$(document).scrollLeft()-_24._outerWidth();
}
if(_27<0){
_27=0;
}
return _27;
};
function _25(){
var top=_23.offset().top+_23._outerHeight();
if(top+_24._outerHeight()>$(window)._outerHeight()+$(document).scrollTop()){
top=_23.offset().top-_24._outerHeight();
}
if(top<$(document).scrollTop()){
top=_23.offset().top+_23._outerHeight();
}
return top;
};
};
function _28(_29){
var _2a=$.data(_29,"combo").options;
var _2b=$.data(_29,"combo").panel;
_2b.panel("close");
_2a.onHidePanel.call(_29);
};
function _2c(_2d,_2e){
var _2f=$.data(_2d,"combo").options;
var _30=$.data(_2d,"combo").combo.find("input.combo-text");
_30.validatebox(_2f);
if(_2e){
_30.validatebox("validate");
}
};
function _31(_32,_33){
var _34=$.data(_32,"combo").options;
var _35=$.data(_32,"combo").combo;
if(_33){
_34.disabled=true;
$(_32).attr("disabled",true);
_35.find(".combo-value").attr("disabled",true);
_35.find(".combo-text").attr("disabled",true);
}else{
_34.disabled=false;
$(_32).removeAttr("disabled");
_35.find(".combo-value").removeAttr("disabled");
_35.find(".combo-text").removeAttr("disabled");
}
};
function _36(_37){
var _38=$.data(_37,"combo").options;
var _39=$.data(_37,"combo").combo;
if(_38.multiple){
_39.find("input.combo-value").remove();
}else{
_39.find("input.combo-value").val("");
}
_39.find("input.combo-text").val("");
};
function _3a(_3b){
var _3c=$.data(_3b,"combo").combo;
return _3c.find("input.combo-text").val();
};
function _3d(_3e,_3f){
var _40=$.data(_3e,"combo").combo;
_40.find("input.combo-text").val(_3f);
_2c(_3e,true);
$.data(_3e,"combo").previousValue=_3f;
};
function _41(_42){
var _43=[];
var _44=$.data(_42,"combo").combo;
_44.find("input.combo-value").each(function(){
_43.push($(this).val());
});
return _43;
};
function _45(_46,_47){
var _48=$.data(_46,"combo").options;
var _49=_41(_46);
var _4a=$.data(_46,"combo").combo;
_4a.find("input.combo-value").remove();
var _4b=$(_46).attr("comboName");
for(var i=0;i<_47.length;i++){
var _4c=$("<input type=\"hidden\" class=\"combo-value\">").appendTo(_4a);
if(_4b){
_4c.attr("name",_4b);
}
_4c.val(_47[i]);
}
var tmp=[];
for(var i=0;i<_49.length;i++){
tmp[i]=_49[i];
}
var aa=[];
for(var i=0;i<_47.length;i++){
for(var j=0;j<tmp.length;j++){
if(_47[i]==tmp[j]){
aa.push(_47[i]);
tmp.splice(j,1);
break;
}
}
}
if(aa.length!=_47.length||_47.length!=_49.length){
if(_48.multiple){
_48.onChange.call(_46,_47,_49);
}else{
_48.onChange.call(_46,_47[0],_49[0]);
}
}
};
function _4d(_4e){
var _4f=_41(_4e);
return _4f[0];
};
function _50(_51,_52){
_45(_51,[_52]);
};
function _53(_54){
var _55=$.data(_54,"combo").options;
var fn=_55.onChange;
_55.onChange=function(){
};
if(_55.multiple){
if(_55.value){
if(typeof _55.value=="object"){
_45(_54,_55.value);
}else{
_50(_54,_55.value);
}
}else{
_45(_54,[]);
}
_55.originalValue=_41(_54);
}else{
_50(_54,_55.value);
_55.originalValue=_55.value;
}
_55.onChange=fn;
};
$.fn.combo=function(_56,_57){
if(typeof _56=="string"){
return $.fn.combo.methods[_56](this,_57);
}
_56=_56||{};
return this.each(function(){
var _58=$.data(this,"combo");
if(_58){
$.extend(_58.options,_56);
}else{
var r=_e(this);
_58=$.data(this,"combo",{options:$.extend({},$.fn.combo.defaults,$.fn.combo.parseOptions(this),_56),combo:r.combo,panel:r.panel,previousValue:null});
$(this).removeAttr("disabled");
}
$("input.combo-text",_58.combo).attr("readonly",!_58.options.editable);
_a(this);
_31(this,_58.options.disabled);
_1(this);
_17(this);
_2c(this);
_53(this);
});
};
$.fn.combo.methods={options:function(jq){
return $.data(jq[0],"combo").options;
},panel:function(jq){
return $.data(jq[0],"combo").panel;
},textbox:function(jq){
return $.data(jq[0],"combo").combo.find("input.combo-text");
},destroy:function(jq){
return jq.each(function(){
_14(this);
});
},resize:function(jq,_59){
return jq.each(function(){
_1(this,_59);
});
},showPanel:function(jq){
return jq.each(function(){
_20(this);
});
},hidePanel:function(jq){
return jq.each(function(){
_28(this);
});
},disable:function(jq){
return jq.each(function(){
_31(this,true);
_17(this);
});
},enable:function(jq){
return jq.each(function(){
_31(this,false);
_17(this);
});
},validate:function(jq){
return jq.each(function(){
_2c(this,true);
});
},isValid:function(jq){
var _5a=$.data(jq[0],"combo").combo.find("input.combo-text");
return _5a.validatebox("isValid");
},clear:function(jq){
return jq.each(function(){
_36(this);
});
},reset:function(jq){
return jq.each(function(){
var _5b=$.data(this,"combo").options;
if(_5b.multiple){
$(this).combo("setValues",_5b.originalValue);
}else{
$(this).combo("setValue",_5b.originalValue);
}
});
},getText:function(jq){
return _3a(jq[0]);
},setText:function(jq,_5c){
return jq.each(function(){
_3d(this,_5c);
});
},getValues:function(jq){
return _41(jq[0]);
},setValues:function(jq,_5d){
return jq.each(function(){
_45(this,_5d);
});
},getValue:function(jq){
return _4d(jq[0]);
},setValue:function(jq,_5e){
return jq.each(function(){
_50(this,_5e);
});
}};
$.fn.combo.parseOptions=function(_5f){
var t=$(_5f);
return $.extend({},$.fn.validatebox.parseOptions(_5f),$.parser.parseOptions(_5f,["width","height","separator",{panelWidth:"number",editable:"boolean",hasDownArrow:"boolean",delay:"number"}]),{panelHeight:(t.attr("panelHeight")=="auto"?"auto":parseInt(t.attr("panelHeight"))||undefined),multiple:(t.attr("multiple")?true:undefined),disabled:(t.attr("disabled")?true:undefined),value:(t.val()||undefined)});
};
$.fn.combo.defaults=$.extend({},$.fn.validatebox.defaults,{width:"auto",height:22,panelWidth:null,panelHeight:200,multiple:false,separator:",",editable:true,disabled:false,hasDownArrow:true,value:"",delay:200,keyHandler:{up:function(){
},down:function(){
},enter:function(){
},query:function(q){
}},onShowPanel:function(){
},onHidePanel:function(){
},onChange:function(_60,_61){
}});
})(jQuery);

