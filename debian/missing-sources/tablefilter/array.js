/**
 * Array utilities
 */

import Str from './string';

export default {
    has: function(arr, val, caseSensitive){
        let sCase = caseSensitive===undefined ? false : caseSensitive;
        for (var i=0; i<arr.length; i++){
            if(Str.matchCase(arr[i].toString(), sCase) == val){
                return true;
            }
        }
        return false;
    }
};
