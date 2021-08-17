/**
 * DOM event utilities
 */

export default {
    add(obj, type, func, capture){
        if(obj.addEventListener){
            obj.addEventListener(type, func, capture);
        }
        else if(obj.attachEvent){
            obj.attachEvent('on'+type, func);
        } else {
            obj['on'+type] = func;
        }
    },
    remove(obj, type, func, capture){
        if(obj.detachEvent){
            obj.detachEvent('on'+type,func);
        }
        else if(obj.removeEventListener){
            obj.removeEventListener(type, func, capture);
        } else {
            obj['on'+type] = null;
        }
    },
    stop(evt){
        if(!evt){
            evt = window.event;
        }
        if(evt.stopPropagation){
            evt.stopPropagation();
        } else {
            evt.cancelBubble = true;
        }
    },
    cancel(evt){
        if(!evt){
            evt = window.event;
        }
        if(evt.preventDefault) {
            evt.preventDefault();
        } else {
            evt.returnValue = false;
        }
    },
    target(evt){
        return (evt && evt.target) || (window.event && window.event.srcElement);
    },
    keyCode(evt){
        return evt.charCode ? evt.charCode :
            (evt.keyCode ? evt.keyCode: (evt.which ? evt.which : 0));
    }
};
