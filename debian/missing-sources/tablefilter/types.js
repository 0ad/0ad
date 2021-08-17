/**
 * Types utilities
 */

const UNDEFINED = void 0;

export default {
    /**
     * Check if argument is an object or a global object
     * @param  {String or Object}  v
     * @return {Boolean}
     */
    isObj(v){
        let isO = false;
        if(typeof v === 'string'){
            if(window[v] && typeof window[v] === 'object'){
                isO = true;
            }
        } else {
            if(v && typeof v === 'object'){
                isO = true;
            }
        }
        return isO;
    },

    /**
     * Check if argument is a function
     * @param  {Function} fn
     * @return {Boolean}
     */
    isFn(fn){
        return (fn && fn.constructor == Function);
    },

    /**
     * Check if argument is an array
     * @param  {Array}  obj
     * @return {Boolean}
     */
    isArray(obj){
        return (obj && obj.constructor == Array);
    },

    /**
     * Determine if argument is undefined
     * @param  {Any}  o
     * @return {Boolean}
     */
    isUndef(o){
        return  o === UNDEFINED;
    },

    /**
     * Determine if argument is null
     * @param  {Any}  o
     * @return {Boolean}
     */
    isNull(o){
        return o === null;
    },

    /**
     * Determine if argument is empty (undefined, null or empty string)
     * @param  {Any}  o
     * @return {Boolean}
     */
    isEmpty(o){
        return this.isUndef(o) || this.isNull(o) || o.length===0;
    }
};
