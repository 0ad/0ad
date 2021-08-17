/**
 * Misc helpers
 */

import Str from './string';

export default {
    removeNbFormat(data, format){
        if(!data){
            return;
        }
        if(!format){
            format = 'us';
        }
        let n = data;
        if(Str.lower(format) === 'us'){
            n =+ n.replace(/[^\d\.-]/g,'');
        } else {
            n =+ n.replace(/[^\d\,-]/g,'').replace(',','.');
        }
        return n;
    }
};
