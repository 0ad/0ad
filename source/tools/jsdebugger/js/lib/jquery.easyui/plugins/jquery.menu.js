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
$(_2).appendTo("body");
$(_2).addClass("menu-top");
$(document).unbind(".menu").bind("mousedown.menu",function(e){
var _3=$("body>div.menu:visible");
var m=$(e.target).closest("div.menu",_3);
if(m.length){
return;
}
$("body>div.menu-top:visible").menu("hide");
});
var _4=_5($(_2));
for(var i=0;i<_4.length;i++){
_6(_4[i]);
}
function _5(_7){
var _8=[];
_7.addClass("menu");
if(!_7[0].style.width){
_7[0].autowidth=true;
}
_8.push(_7);
if(!_7.hasClass("menu-content")){
_7.children("div").each(function(){
var _9=$(this).children("div");
if(_9.length){
_9.insertAfter(_2);
this.submenu=_9;
var mm=_5(_9);
_8=_8.concat(mm);
}
});
}
return _8;
};
function _6(_a){
if(!_a.hasClass("menu-content")){
_a.children("div").each(function(){
var _b=$(this);
if(_b.hasClass("menu-sep")){
}else{
var _c=$.extend({},$.parser.parseOptions(this,["name","iconCls","href"]),{disabled:(_b.attr("disabled")?true:undefined)});
_b.attr("name",_c.name||"").attr("href",_c.href||"");
var _d=_b.addClass("menu-item").html();
_b.empty().append($("<div class=\"menu-text\"></div>").html(_d));
if(_c.iconCls){
$("<div class=\"menu-icon\"></div>").addClass(_c.iconCls).appendTo(_b);
}
if(_c.disabled){
_e(_2,_b[0],true);
}
if(_b[0].submenu){
$("<div class=\"menu-rightarrow\"></div>").appendTo(_b);
}
_b._outerHeight(22);
_f(_2,_b);
}
});
$("<div class=\"menu-line\"></div>").prependTo(_a);
}
_10(_2,_a);
_a.hide();
_11(_2,_a);
};
};
function _10(_12,_13){
var _14=$.data(_12,"menu").options;
var d=_13.css("display");
_13.css({display:"block",left:-10000});
var _15=_13._outerWidth();
var _16=0;
_13.find("div.menu-text").each(function(){
if(_16<$(this)._outerWidth()){
_16=$(this)._outerWidth();
}
});
_16+=65;
_13._outerWidth(Math.max(_15,_16,_14.minWidth));
_13.css("display",d);
};
function _11(_17,_18){
var _19=$.data(_17,"menu");
_18.unbind(".menu").bind("mouseenter.menu",function(){
if(_19.timer){
clearTimeout(_19.timer);
_19.timer=null;
}
}).bind("mouseleave.menu",function(){
_19.timer=setTimeout(function(){
_1a(_17);
},100);
});
};
function _f(_1b,_1c){
_1c.unbind(".menu");
_1c.bind("click.menu",function(){
if($(this).hasClass("menu-item-disabled")){
return;
}
if(!this.submenu){
_1a(_1b);
var _1d=$(this).attr("href");
if(_1d){
location.href=_1d;
}
}
var _1e=$(_1b).menu("getItem",this);
$.data(_1b,"menu").options.onClick.call(_1b,_1e);
}).bind("mouseenter.menu",function(e){
_1c.siblings().each(function(){
if(this.submenu){
_21(this.submenu);
}
$(this).removeClass("menu-active");
});
_1c.addClass("menu-active");
if($(this).hasClass("menu-item-disabled")){
_1c.addClass("menu-active-disabled");
return;
}
var _1f=_1c[0].submenu;
if(_1f){
$(_1b).menu("show",{menu:_1f,parent:_1c});
}
}).bind("mouseleave.menu",function(e){
_1c.removeClass("menu-active menu-active-disabled");
var _20=_1c[0].submenu;
if(_20){
if(e.pageX>=parseInt(_20.css("left"))){
_1c.addClass("menu-active");
}else{
_21(_20);
}
}else{
_1c.removeClass("menu-active");
}
});
};
function _1a(_22){
var _23=$.data(_22,"menu");
if(_23){
if($(_22).is(":visible")){
_21($(_22));
_23.options.onHide.call(_22);
}
}
return false;
};
function _24(_25,_26){
var _27,top;
var _28=$(_26.menu||_25);
if(_28.hasClass("menu-top")){
var _29=$.data(_25,"menu").options;
_27=_29.left;
top=_29.top;
if(_26.alignTo){
var at=$(_26.alignTo);
_27=at.offset().left;
top=at.offset().top+at._outerHeight();
}
if(_26.left!=undefined){
_27=_26.left;
}
if(_26.top!=undefined){
top=_26.top;
}
if(_27+_28.outerWidth()>$(window)._outerWidth()+$(document)._scrollLeft()){
_27=$(window)._outerWidth()+$(document).scrollLeft()-_28.outerWidth()-5;
}
if(top+_28.outerHeight()>$(window)._outerHeight()+$(document).scrollTop()){
top-=_28.outerHeight();
}
}else{
var _2a=_26.parent;
_27=_2a.offset().left+_2a.outerWidth()-2;
if(_27+_28.outerWidth()+5>$(window)._outerWidth()+$(document).scrollLeft()){
_27=_2a.offset().left-_28.outerWidth()+2;
}
var top=_2a.offset().top-3;
if(top+_28.outerHeight()>$(window)._outerHeight()+$(document).scrollTop()){
top=$(window)._outerHeight()+$(document).scrollTop()-_28.outerHeight()-5;
}
}
_28.css({left:_27,top:top});
_28.show(0,function(){
if(!_28[0].shadow){
_28[0].shadow=$("<div class=\"menu-shadow\"></div>").insertAfter(_28);
}
_28[0].shadow.css({display:"block",zIndex:$.fn.menu.defaults.zIndex++,left:_28.css("left"),top:_28.css("top"),width:_28.outerWidth(),height:_28.outerHeight()});
_28.css("z-index",$.fn.menu.defaults.zIndex++);
if(_28.hasClass("menu-top")){
$.data(_28[0],"menu").options.onShow.call(_28[0]);
}
});
};
function _21(_2b){
if(!_2b){
return;
}
_2c(_2b);
_2b.find("div.menu-item").each(function(){
if(this.submenu){
_21(this.submenu);
}
$(this).removeClass("menu-active");
});
function _2c(m){
m.stop(true,true);
if(m[0].shadow){
m[0].shadow.hide();
}
m.hide();
};
};
function _2d(_2e,_2f){
var _30=null;
var tmp=$("<div></div>");
function _31(_32){
_32.children("div.menu-item").each(function(){
var _33=$(_2e).menu("getItem",this);
var s=tmp.empty().html(_33.text).text();
if(_2f==$.trim(s)){
_30=_33;
}else{
if(this.submenu&&!_30){
_31(this.submenu);
}
}
});
};
_31($(_2e));
tmp.remove();
return _30;
};
function _e(_34,_35,_36){
var t=$(_35);
if(_36){
t.addClass("menu-item-disabled");
if(_35.onclick){
_35.onclick1=_35.onclick;
_35.onclick=null;
}
}else{
t.removeClass("menu-item-disabled");
if(_35.onclick1){
_35.onclick=_35.onclick1;
_35.onclick1=null;
}
}
};
function _37(_38,_39){
var _3a=$(_38);
if(_39.parent){
if(!_39.parent.submenu){
var _3b=$("<div class=\"menu\"><div class=\"menu-line\"></div></div>").appendTo("body");
_3b[0].autowidth=true;
_3b.hide();
_39.parent.submenu=_3b;
$("<div class=\"menu-rightarrow\"></div>").appendTo(_39.parent);
}
_3a=_39.parent.submenu;
}
var _3c=$("<div class=\"menu-item\"></div>").appendTo(_3a);
$("<div class=\"menu-text\"></div>").html(_39.text).appendTo(_3c);
if(_39.iconCls){
$("<div class=\"menu-icon\"></div>").addClass(_39.iconCls).appendTo(_3c);
}
if(_39.id){
_3c.attr("id",_39.id);
}
if(_39.href){
_3c.attr("href",_39.href);
}
if(_39.name){
_3c.attr("name",_39.name);
}
if(_39.onclick){
if(typeof _39.onclick=="string"){
_3c.attr("onclick",_39.onclick);
}else{
_3c[0].onclick=eval(_39.onclick);
}
}
if(_39.handler){
_3c[0].onclick=eval(_39.handler);
}
_f(_38,_3c);
if(_39.disabled){
_e(_38,_3c[0],true);
}
_11(_38,_3a);
_10(_38,_3a);
};
function _3d(_3e,_3f){
function _40(el){
if(el.submenu){
el.submenu.children("div.menu-item").each(function(){
_40(this);
});
var _41=el.submenu[0].shadow;
if(_41){
_41.remove();
}
el.submenu.remove();
}
$(el).remove();
};
_40(_3f);
};
function _42(_43){
$(_43).children("div.menu-item").each(function(){
_3d(_43,this);
});
if(_43.shadow){
_43.shadow.remove();
}
$(_43).remove();
};
$.fn.menu=function(_44,_45){
if(typeof _44=="string"){
return $.fn.menu.methods[_44](this,_45);
}
_44=_44||{};
return this.each(function(){
var _46=$.data(this,"menu");
if(_46){
$.extend(_46.options,_44);
}else{
_46=$.data(this,"menu",{options:$.extend({},$.fn.menu.defaults,$.fn.menu.parseOptions(this),_44)});
_1(this);
}
$(this).css({left:_46.options.left,top:_46.options.top});
});
};
$.fn.menu.methods={options:function(jq){
return $.data(jq[0],"menu").options;
},show:function(jq,pos){
return jq.each(function(){
_24(this,pos);
});
},hide:function(jq){
return jq.each(function(){
_1a(this);
});
},destroy:function(jq){
return jq.each(function(){
_42(this);
});
},setText:function(jq,_47){
return jq.each(function(){
$(_47.target).children("div.menu-text").html(_47.text);
});
},setIcon:function(jq,_48){
return jq.each(function(){
var _49=$(this).menu("getItem",_48.target);
if(_49.iconCls){
$(_49.target).children("div.menu-icon").removeClass(_49.iconCls).addClass(_48.iconCls);
}else{
$("<div class=\"menu-icon\"></div>").addClass(_48.iconCls).appendTo(_48.target);
}
});
},getItem:function(jq,_4a){
var t=$(_4a);
var _4b={target:_4a,id:t.attr("id"),text:$.trim(t.children("div.menu-text").html()),disabled:t.hasClass("menu-item-disabled"),href:t.attr("href"),name:t.attr("name"),onclick:_4a.onclick};
var _4c=t.children("div.menu-icon");
if(_4c.length){
var cc=[];
var aa=_4c.attr("class").split(" ");
for(var i=0;i<aa.length;i++){
if(aa[i]!="menu-icon"){
cc.push(aa[i]);
}
}
_4b.iconCls=cc.join(" ");
}
return _4b;
},findItem:function(jq,_4d){
return _2d(jq[0],_4d);
},appendItem:function(jq,_4e){
return jq.each(function(){
_37(this,_4e);
});
},removeItem:function(jq,_4f){
return jq.each(function(){
_3d(this,_4f);
});
},enableItem:function(jq,_50){
return jq.each(function(){
_e(this,_50,false);
});
},disableItem:function(jq,_51){
return jq.each(function(){
_e(this,_51,true);
});
}};
$.fn.menu.parseOptions=function(_52){
return $.extend({},$.parser.parseOptions(_52,["left","top",{minWidth:"number"}]));
};
$.fn.menu.defaults={zIndex:110000,left:0,top:0,minWidth:120,onShow:function(){
},onHide:function(){
},onClick:function(_53){
}};
})(jQuery);

