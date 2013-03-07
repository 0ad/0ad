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
var _3=$.data(_2,"pagination");
var _4=_3.options;
var bb=_3.bb={};
var _5=$(_2).addClass("pagination").html("<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\"><tr></tr></table>");
var tr=_5.find("tr");
function _6(_7){
var _8=_4.nav[_7];
var a=$("<a href=\"javascript:void(0)\"></a>").appendTo(tr);
a.wrap("<td></td>");
a.linkbutton({iconCls:_8.iconCls,plain:true}).unbind(".pagination").bind("click.pagination",function(){
_8.handler.call(_2);
});
return a;
};
if(_4.showPageList){
var ps=$("<select class=\"pagination-page-list\"></select>");
ps.bind("change",function(){
_4.pageSize=parseInt($(this).val());
_4.onChangePageSize.call(_2,_4.pageSize);
_b(_2,_4.pageNumber);
});
for(var i=0;i<_4.pageList.length;i++){
$("<option></option>").text(_4.pageList[i]).appendTo(ps);
}
$("<td></td>").append(ps).appendTo(tr);
$("<td><div class=\"pagination-btn-separator\"></div></td>").appendTo(tr);
}
bb.first=_6("first");
bb.prev=_6("prev");
$("<td><div class=\"pagination-btn-separator\"></div></td>").appendTo(tr);
$("<span style=\"padding-left:6px;\"></span>").html(_4.beforePageText).appendTo(tr).wrap("<td></td>");
bb.num=$("<input class=\"pagination-num\" type=\"text\" value=\"1\" size=\"2\">").appendTo(tr).wrap("<td></td>");
bb.num.unbind(".pagination").bind("keydown.pagination",function(e){
if(e.keyCode==13){
var _9=parseInt($(this).val())||1;
_b(_2,_9);
return false;
}
});
bb.after=$("<span style=\"padding-right:6px;\"></span>").appendTo(tr).wrap("<td></td>");
$("<td><div class=\"pagination-btn-separator\"></div></td>").appendTo(tr);
bb.next=_6("next");
bb.last=_6("last");
if(_4.showRefresh){
$("<td><div class=\"pagination-btn-separator\"></div></td>").appendTo(tr);
bb.refresh=_6("refresh");
}
if(_4.buttons){
$("<td><div class=\"pagination-btn-separator\"></div></td>").appendTo(tr);
for(var i=0;i<_4.buttons.length;i++){
var _a=_4.buttons[i];
if(_a=="-"){
$("<td><div class=\"pagination-btn-separator\"></div></td>").appendTo(tr);
}else{
var td=$("<td></td>").appendTo(tr);
$("<a href=\"javascript:void(0)\"></a>").appendTo(td).linkbutton($.extend(_a,{plain:true})).bind("click",eval(_a.handler||function(){
}));
}
}
}
$("<div class=\"pagination-info\"></div>").appendTo(_5);
$("<div style=\"clear:both;\"></div>").appendTo(_5);
};
function _b(_c,_d){
var _e=$.data(_c,"pagination").options;
var _f=Math.ceil(_e.total/_e.pageSize)||1;
_e.pageNumber=_d;
if(_e.pageNumber<1){
_e.pageNumber=1;
}
if(_e.pageNumber>_f){
_e.pageNumber=_f;
}
_10(_c,{pageNumber:_e.pageNumber});
_e.onSelectPage.call(_c,_e.pageNumber,_e.pageSize);
};
function _10(_11,_12){
var _13=$.data(_11,"pagination").options;
var bb=$.data(_11,"pagination").bb;
$.extend(_13,_12||{});
var ps=$(_11).find("select.pagination-page-list");
if(ps.length){
ps.val(_13.pageSize+"");
_13.pageSize=parseInt(ps.val());
}
var _14=Math.ceil(_13.total/_13.pageSize)||1;
bb.num.val(_13.pageNumber);
bb.after.html(_13.afterPageText.replace(/{pages}/,_14));
var _15=_13.displayMsg;
_15=_15.replace(/{from}/,_13.total==0?0:_13.pageSize*(_13.pageNumber-1)+1);
_15=_15.replace(/{to}/,Math.min(_13.pageSize*(_13.pageNumber),_13.total));
_15=_15.replace(/{total}/,_13.total);
$(_11).find("div.pagination-info").html(_15);
bb.first.add(bb.prev).linkbutton({disabled:(_13.pageNumber==1)});
bb.next.add(bb.last).linkbutton({disabled:(_13.pageNumber==_14)});
_16(_11,_13.loading);
};
function _16(_17,_18){
var _19=$.data(_17,"pagination").options;
var bb=$.data(_17,"pagination").bb;
_19.loading=_18;
if(_19.showRefresh){
if(_19.loading){
bb.refresh.linkbutton({iconCls:"pagination-loading"});
}else{
bb.refresh.linkbutton({iconCls:"pagination-load"});
}
}
};
$.fn.pagination=function(_1a,_1b){
if(typeof _1a=="string"){
return $.fn.pagination.methods[_1a](this,_1b);
}
_1a=_1a||{};
return this.each(function(){
var _1c;
var _1d=$.data(this,"pagination");
if(_1d){
_1c=$.extend(_1d.options,_1a);
}else{
_1c=$.extend({},$.fn.pagination.defaults,$.fn.pagination.parseOptions(this),_1a);
$.data(this,"pagination",{options:_1c});
}
_1(this);
_10(this);
});
};
$.fn.pagination.methods={options:function(jq){
return $.data(jq[0],"pagination").options;
},loading:function(jq){
return jq.each(function(){
_16(this,true);
});
},loaded:function(jq){
return jq.each(function(){
_16(this,false);
});
},refresh:function(jq,_1e){
return jq.each(function(){
_10(this,_1e);
});
},select:function(jq,_1f){
return jq.each(function(){
_b(this,_1f);
});
}};
$.fn.pagination.parseOptions=function(_20){
var t=$(_20);
return $.extend({},$.parser.parseOptions(_20,[{total:"number",pageSize:"number",pageNumber:"number"},{loading:"boolean",showPageList:"boolean",showRefresh:"boolean"}]),{pageList:(t.attr("pageList")?eval(t.attr("pageList")):undefined)});
};
$.fn.pagination.defaults={total:1,pageSize:10,pageNumber:1,pageList:[10,20,30,50],loading:false,buttons:null,showPageList:true,showRefresh:true,onSelectPage:function(_21,_22){
},onBeforeRefresh:function(_23,_24){
},onRefresh:function(_25,_26){
},onChangePageSize:function(_27){
},beforePageText:"Page",afterPageText:"of {pages}",displayMsg:"Displaying {from} to {to} of {total} items",nav:{first:{iconCls:"pagination-first",handler:function(){
var _28=$(this).pagination("options");
if(_28.pageNumber>1){
$(this).pagination("select",1);
}
}},prev:{iconCls:"pagination-prev",handler:function(){
var _29=$(this).pagination("options");
if(_29.pageNumber>1){
$(this).pagination("select",_29.pageNumber-1);
}
}},next:{iconCls:"pagination-next",handler:function(){
var _2a=$(this).pagination("options");
var _2b=Math.ceil(_2a.total/_2a.pageSize);
if(_2a.pageNumber<_2b){
$(this).pagination("select",_2a.pageNumber+1);
}
}},last:{iconCls:"pagination-last",handler:function(){
var _2c=$(this).pagination("options");
var _2d=Math.ceil(_2c.total/_2c.pageSize);
if(_2c.pageNumber<_2d){
$(this).pagination("select",_2d);
}
}},refresh:{iconCls:"pagination-refresh",handler:function(){
var _2e=$(this).pagination("options");
if(_2e.onBeforeRefresh.call(this,_2e.pageNumber,_2e.pageSize)!=false){
$(this).pagination("select",_2e.pageNumber);
_2e.onRefresh.call(this,_2e.pageNumber,_2e.pageSize);
}
}}}};
})(jQuery);

