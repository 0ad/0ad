import Cookie from '../cookie';

export class Store{

    /**
     * Store, persistence manager
     * @param {Object} tf TableFilter instance
     *
     * TODO: use localStorage and fallback to cookie persistence
     */
    constructor(tf) {
        var f = tf.config();

        this.duration = !isNaN(f.set_cookie_duration) ?
            parseInt(f.set_cookie_duration, 10) : 100000;

        this.tf = tf;
    }

    /**
     * Store filters' values in cookie
     * @param {String} cookie name
     */
    saveFilterValues(name){
        var tf = this.tf;
        var fltValues = [];
        //store filters' values
        for(var i=0; i<tf.fltIds.length; i++){
            var value = tf.getFilterValue(i);
            if (value === ''){
                value = ' ';
            }
            fltValues.push(value);
        }
        //adds array size
        fltValues.push(tf.fltIds.length);

        //writes cookie
        Cookie.write(
            name,
            fltValues.join(tf.separator),
            this.duration
        );
    }

    /**
     * Retrieve filters' values from cookie
     * @param {String} cookie name
     * @return {Array}
     */
    getFilterValues(name){
        var flts = Cookie.read(name);
        var rgx = new RegExp(this.tf.separator, 'g');
        // filters' values array
        return flts.split(rgx);
    }

    /**
     * Store page number in cookie
     * @param {String} cookie name
     */
    savePageNb(name){
        Cookie.write(
            name,
            this.tf.feature('paging').currentPageNb,
            this.duration
        );
    }

    /**
     * Retrieve page number from cookie
     * @param {String} cookie name
     * @return {String}
     */
    getPageNb(name){
        return Cookie.read(name);
    }

    /**
     * Store page length in cookie
     * @param {String} cookie name
     */
    savePageLength(name){
        Cookie.write(
            name,
            this.tf.feature('paging').resultsPerPageSlc.selectedIndex,
            this.duration
        );
    }

    /**
     * Retrieve page length from cookie
     * @param {String} cookie name
     * @return {String}
     */
    getPageLength(name){
        return Cookie.read(name);
    }

}
