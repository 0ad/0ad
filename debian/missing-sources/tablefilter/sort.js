import Str from './string';

export default {
   ignoreCase(a, b){
        let x = Str.lower(a);
        let y = Str.lower(b);
        return ((x < y) ? -1 : ((x > y) ? 1 : 0));
    }
};
