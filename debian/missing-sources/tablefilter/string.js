/**
 * String utilities
 */

export default {

    lower(text){
        return text.toLowerCase();
    },

    upper(text){
        return text.toUpperCase();
    },

    trim(text){
        if (text.trim){
            return text.trim();
        }
        return text.replace(/^\s*|\s*$/g, '');
    },

    isEmpty(text){
        return this.trim(text) === '';
    },

    rgxEsc(text){
        let chars = /[-\/\\^$*+?.()|[\]{}]/g;
        let escMatch = '\\$&';
        return String(text).replace(chars, escMatch);
    },

    matchCase(text, mc){
        if(!mc){
            return this.lower(text);
        }
        return text;
    }

};
