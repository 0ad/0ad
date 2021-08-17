/**
 * Cookie utilities
 */

export default {

    write(name, value, hours){
        let expire = '';
        if(hours){
            expire = new Date((new Date()).getTime() + hours * 3600000);
            expire = '; expires=' + expire.toGMTString();
        }
        document.cookie = name + '=' + escape(value) + expire;
    },

    read(name){
        let cookieValue = '',
            search = name + '=';
        if(document.cookie.length > 0){
            let cookie = document.cookie,
                offset = cookie.indexOf(search);
            if(offset !== -1){
                offset += search.length;
                let end = cookie.indexOf(';', offset);
                if(end === -1){
                    end = cookie.length;
                }
                cookieValue = unescape(cookie.substring(offset, end));
            }
        }
        return cookieValue;
    },

    remove(name){
        this.write(name, '', -1);
    },

    valueToArray(name, separator){
        if(!separator){
            separator = ',';
        }
        //reads the cookie
        let val = this.read(name);
        //creates an array with filters' values
        let arr = val.split(separator);
        return arr;
    },

    getValueByIndex(name, index, separator){
        if(!separator){
            separator = ',';
        }
        //reads the cookie
        let val = this.valueToArray(name, separator);
        return val[index];
    }

};
