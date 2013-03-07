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
function _2(e){
var _3=$.data(e.data.target,"draggable");
var _4=_3.options;
var _5=_3.proxy;
var _6=e.data;
var _7=_6.startLeft+e.pageX-_6.startX;
var _8=_6.startTop+e.pageY-_6.startY;
if(_5){
if(_5.parent()[0]==document.body){
if(_4.deltaX!=null&&_4.deltaX!=undefined){
_7=e.pageX+_4.deltaX;
}else{
_7=e.pageX-e.data.offsetWidth;
}
if(_4.deltaY!=null&&_4.deltaY!=undefined){
_8=e.pageY+_4.deltaY;
}else{
_8=e.pageY-e.data.offsetHeight;
}
}else{
if(_4.deltaX!=null&&_4.deltaX!=undefined){
_7+=e.data.offsetWidth+_4.deltaX;
}
if(_4.deltaY!=null&&_4.deltaY!=undefined){
_8+=e.data.offsetHeight+_4.deltaY;
}
}
}
if(e.data.parent!=document.body){
_7+=$(e.data.parent).scrollLeft();
_8+=$(e.data.parent).scrollTop();
}
if(_4.axis=="h"){
_6.left=_7;
}else{
if(_4.axis=="v"){
_6.top=_8;
}else{
_6.left=_7;
_6.top=_8;
}
}
};
function _9(e){
var _a=$.data(e.data.target,"draggable");
var _b=_a.options;
var _c=_a.proxy;
if(!_c){
_c=$(e.data.target);
}
_c.css({left:e.data.left,top:e.data.top});
$("body").css("cursor",_b.cursor);
};
function _d(e){
_1=true;
var _e=$.data(e.data.target,"draggable");
var _f=_e.options;
var _10=$(".droppable").filter(function(){
return e.data.target!=this;
}).filter(function(){
var _11=$.data(this,"droppable").options.accept;
if(_11){
return $(_11).filter(function(){
return this==e.data.target;
}).length>0;
}else{
return true;
}
});
_e.droppables=_10;
var _12=_e.proxy;
if(!_12){
if(_f.proxy){
if(_f.proxy=="clone"){
_12=$(e.data.target).clone().insertAfter(e.data.target);
}else{
_12=_f.proxy.call(e.data.target,e.data.target);
}
_e.proxy=_12;
}else{
_12=$(e.data.target);
}
}
_12.css("position","absolute");
_2(e);
_9(e);
_f.onStartDrag.call(e.data.target,e);
return false;
};
function _13(e){
var _14=$.data(e.data.target,"draggable");
_2(e);
if(_14.options.onDrag.call(e.data.target,e)!=false){
_9(e);
}
var _15=e.data.target;
_14.droppables.each(function(){
var _16=$(this);
if(_16.droppable("options").disabled){
return;
}
var p2=_16.offset();
if(e.pageX>p2.left&&e.pageX<p2.left+_16.outerWidth()&&e.pageY>p2.top&&e.pageY<p2.top+_16.outerHeight()){
if(!this.entered){
$(this).trigger("_dragenter",[_15]);
this.entered=true;
}
$(this).trigger("_dragover",[_15]);
}else{
if(this.entered){
$(this).trigger("_dragleave",[_15]);
this.entered=false;
}
}
});
return false;
};
function _17(e){
_1=false;
_13(e);
var _18=$.data(e.data.target,"draggable");
var _19=_18.proxy;
var _1a=_18.options;
if(_1a.revert){
if(_1b()==true){
$(e.data.target).css({position:e.data.startPosition,left:e.data.startLeft,top:e.data.startTop});
}else{
if(_19){
var _1c,top;
if(_19.parent()[0]==document.body){
_1c=e.data.startX-e.data.offsetWidth;
top=e.data.startY-e.data.offsetHeight;
}else{
_1c=e.data.startLeft;
top=e.data.startTop;
}
_19.animate({left:_1c,top:top},function(){
_1d();
});
}else{
$(e.data.target).animate({left:e.data.startLeft,top:e.data.startTop},function(){
$(e.data.target).css("position",e.data.startPosition);
});
}
}
}else{
$(e.data.target).css({position:"absolute",left:e.data.left,top:e.data.top});
_1b();
}
_1a.onStopDrag.call(e.data.target,e);
$(document).unbind(".draggable");
setTimeout(function(){
$("body").css("cursor","");
},100);
function _1d(){
if(_19){
_19.remove();
}
_18.proxy=null;
};
function _1b(){
var _1e=false;
_18.droppables.each(function(){
var _1f=$(this);
if(_1f.droppable("options").disabled){
return;
}
var p2=_1f.offset();
if(e.pageX>p2.left&&e.pageX<p2.left+_1f.outerWidth()&&e.pageY>p2.top&&e.pageY<p2.top+_1f.outerHeight()){
if(_1a.revert){
$(e.data.target).css({position:e.data.startPosition,left:e.data.startLeft,top:e.data.startTop});
}
_1d();
$(this).trigger("_drop",[e.data.target]);
_1e=true;
this.entered=false;
return false;
}
});
if(!_1e&&!_1a.revert){
_1d();
}
return _1e;
};
return false;
};
$.fn.draggable=function(_20,_21){
if(typeof _20=="string"){
return $.fn.draggable.methods[_20](this,_21);
}
return this.each(function(){
var _22;
var _23=$.data(this,"draggable");
if(_23){
_23.handle.unbind(".draggable");
_22=$.extend(_23.options,_20);
}else{
_22=$.extend({},$.fn.draggable.defaults,$.fn.draggable.parseOptions(this),_20||{});
}
if(_22.disabled==true){
$(this).css("cursor","");
return;
}
var _24=null;
if(typeof _22.handle=="undefined"||_22.handle==null){
_24=$(this);
}else{
_24=(typeof _22.handle=="string"?$(_22.handle,this):_22.handle);
}
$.data(this,"draggable",{options:_22,handle:_24});
_24.unbind(".draggable").bind("mousemove.draggable",{target:this},function(e){
if(_1){
return;
}
var _25=$.data(e.data.target,"draggable").options;
if(_26(e)){
$(this).css("cursor",_25.cursor);
}else{
$(this).css("cursor","");
}
}).bind("mouseleave.draggable",{target:this},function(e){
$(this).css("cursor","");
}).bind("mousedown.draggable",{target:this},function(e){
if(_26(e)==false){
return;
}
$(this).css("cursor","");
var _27=$(e.data.target).position();
var _28=$(e.data.target).offset();
var _29={startPosition:$(e.data.target).css("position"),startLeft:_27.left,startTop:_27.top,left:_27.left,top:_27.top,startX:e.pageX,startY:e.pageY,offsetWidth:(e.pageX-_28.left),offsetHeight:(e.pageY-_28.top),target:e.data.target,parent:$(e.data.target).parent()[0]};
$.extend(e.data,_29);
var _2a=$.data(e.data.target,"draggable").options;
if(_2a.onBeforeDrag.call(e.data.target,e)==false){
return;
}
$(document).bind("mousedown.draggable",e.data,_d);
$(document).bind("mousemove.draggable",e.data,_13);
$(document).bind("mouseup.draggable",e.data,_17);
});
function _26(e){
var _2b=$.data(e.data.target,"draggable");
var _2c=_2b.handle;
var _2d=$(_2c).offset();
var _2e=$(_2c).outerWidth();
var _2f=$(_2c).outerHeight();
var t=e.pageY-_2d.top;
var r=_2d.left+_2e-e.pageX;
var b=_2d.top+_2f-e.pageY;
var l=e.pageX-_2d.left;
return Math.min(t,r,b,l)>_2b.options.edge;
};
});
};
$.fn.draggable.methods={options:function(jq){
return $.data(jq[0],"draggable").options;
},proxy:function(jq){
return $.data(jq[0],"draggable").proxy;
},enable:function(jq){
return jq.each(function(){
$(this).draggable({disabled:false});
});
},disable:function(jq){
return jq.each(function(){
$(this).draggable({disabled:true});
});
}};
$.fn.draggable.parseOptions=function(_30){
var t=$(_30);
return $.extend({},$.parser.parseOptions(_30,["cursor","handle","axis",{"revert":"boolean","deltaX":"number","deltaY":"number","edge":"number"}]),{disabled:(t.attr("disabled")?true:undefined)});
};
$.fn.draggable.defaults={proxy:null,revert:false,cursor:"move",deltaX:null,deltaY:null,handle:null,disabled:false,edge:0,axis:null,onBeforeDrag:function(e){
},onStartDrag:function(e){
},onDrag:function(e){
},onStopDrag:function(e){
}};
})(jQuery);

