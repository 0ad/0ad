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
var _1=false;
$.fn.resizable=function(_2,_3){
if(typeof _2=="string"){
return $.fn.resizable.methods[_2](this,_3);
}
function _4(e){
var _5=e.data;
var _6=$.data(_5.target,"resizable").options;
if(_5.dir.indexOf("e")!=-1){
var _7=_5.startWidth+e.pageX-_5.startX;
_7=Math.min(Math.max(_7,_6.minWidth),_6.maxWidth);
_5.width=_7;
}
if(_5.dir.indexOf("s")!=-1){
var _8=_5.startHeight+e.pageY-_5.startY;
_8=Math.min(Math.max(_8,_6.minHeight),_6.maxHeight);
_5.height=_8;
}
if(_5.dir.indexOf("w")!=-1){
_5.width=_5.startWidth-e.pageX+_5.startX;
if(_5.width>=_6.minWidth&&_5.width<=_6.maxWidth){
_5.left=_5.startLeft+e.pageX-_5.startX;
}
}
if(_5.dir.indexOf("n")!=-1){
_5.height=_5.startHeight-e.pageY+_5.startY;
if(_5.height>=_6.minHeight&&_5.height<=_6.maxHeight){
_5.top=_5.startTop+e.pageY-_5.startY;
}
}
};
function _9(e){
var _a=e.data;
var _b=_a.target;
$(_b).css({left:_a.left,top:_a.top});
$(_b)._outerWidth(_a.width)._outerHeight(_a.height);
};
function _c(e){
_1=true;
$.data(e.data.target,"resizable").options.onStartResize.call(e.data.target,e);
return false;
};
function _d(e){
_4(e);
if($.data(e.data.target,"resizable").options.onResize.call(e.data.target,e)!=false){
_9(e);
}
return false;
};
function _e(e){
_1=false;
_4(e,true);
_9(e);
$.data(e.data.target,"resizable").options.onStopResize.call(e.data.target,e);
$(document).unbind(".resizable");
$("body").css("cursor","");
return false;
};
return this.each(function(){
var _f=null;
var _10=$.data(this,"resizable");
if(_10){
$(this).unbind(".resizable");
_f=$.extend(_10.options,_2||{});
}else{
_f=$.extend({},$.fn.resizable.defaults,$.fn.resizable.parseOptions(this),_2||{});
$.data(this,"resizable",{options:_f});
}
if(_f.disabled==true){
return;
}
$(this).bind("mousemove.resizable",{target:this},function(e){
if(_1){
return;
}
var dir=_11(e);
if(dir==""){
$(e.data.target).css("cursor","");
}else{
$(e.data.target).css("cursor",dir+"-resize");
}
}).bind("mouseleave.resizable",{target:this},function(e){
$(e.data.target).css("cursor","");
}).bind("mousedown.resizable",{target:this},function(e){
var dir=_11(e);
if(dir==""){
return;
}
function _12(css){
var val=parseInt($(e.data.target).css(css));
if(isNaN(val)){
return 0;
}else{
return val;
}
};
var _13={target:e.data.target,dir:dir,startLeft:_12("left"),startTop:_12("top"),left:_12("left"),top:_12("top"),startX:e.pageX,startY:e.pageY,startWidth:$(e.data.target).outerWidth(),startHeight:$(e.data.target).outerHeight(),width:$(e.data.target).outerWidth(),height:$(e.data.target).outerHeight(),deltaWidth:$(e.data.target).outerWidth()-$(e.data.target).width(),deltaHeight:$(e.data.target).outerHeight()-$(e.data.target).height()};
$(document).bind("mousedown.resizable",_13,_c);
$(document).bind("mousemove.resizable",_13,_d);
$(document).bind("mouseup.resizable",_13,_e);
$("body").css("cursor",dir+"-resize");
});
function _11(e){
var tt=$(e.data.target);
var dir="";
var _14=tt.offset();
var _15=tt.outerWidth();
var _16=tt.outerHeight();
var _17=_f.edge;
if(e.pageY>_14.top&&e.pageY<_14.top+_17){
dir+="n";
}else{
if(e.pageY<_14.top+_16&&e.pageY>_14.top+_16-_17){
dir+="s";
}
}
if(e.pageX>_14.left&&e.pageX<_14.left+_17){
dir+="w";
}else{
if(e.pageX<_14.left+_15&&e.pageX>_14.left+_15-_17){
dir+="e";
}
}
var _18=_f.handles.split(",");
for(var i=0;i<_18.length;i++){
var _19=_18[i].replace(/(^\s*)|(\s*$)/g,"");
if(_19=="all"||_19==dir){
return dir;
}
}
return "";
};
});
};
$.fn.resizable.methods={options:function(jq){
return $.data(jq[0],"resizable").options;
},enable:function(jq){
return jq.each(function(){
$(this).resizable({disabled:false});
});
},disable:function(jq){
return jq.each(function(){
$(this).resizable({disabled:true});
});
}};
$.fn.resizable.parseOptions=function(_1a){
var t=$(_1a);
return $.extend({},$.parser.parseOptions(_1a,["handles",{minWidth:"number",minHeight:"number",maxWidth:"number",maxHeight:"number",edge:"number"}]),{disabled:(t.attr("disabled")?true:undefined)});
};
$.fn.resizable.defaults={disabled:false,handles:"n, e, s, w, ne, se, sw, nw, all",minWidth:10,minHeight:10,maxWidth:10000,maxHeight:10000,edge:5,onStartResize:function(e){
},onResize:function(e){
},onStopResize:function(e){
}};
})(jQuery);

