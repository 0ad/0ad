/**
 * Date utilities
 */

export default {
    isValid(dateStr, format){
        if(!format) {
            format = 'DMY';
        }
        format = format.toUpperCase();
        if(format.length != 3) {
            if(format==='DDMMMYYYY'){
                let d = this.format(dateStr, format);
                dateStr = d.getDate() +'/'+ (d.getMonth()+1) +'/'+
                    d.getFullYear();
                format = 'DMY';
            }
        }
        if((format.indexOf('M') === -1) || (format.indexOf('D') === -1) ||
            (format.indexOf('Y') === -1)){
            format = 'DMY';
        }
        let reg1, reg2;
        // If the year is first
        if(format.substring(0, 1) == 'Y') {
              reg1 = /^\d{2}(\-|\/|\.)\d{1,2}\1\d{1,2}$/;
              reg2 = /^\d{4}(\-|\/|\.)\d{1,2}\1\d{1,2}$/;
        } else if(format.substring(1, 2) == 'Y') { // If the year is second
              reg1 = /^\d{1,2}(\-|\/|\.)\d{2}\1\d{1,2}$/;
              reg2 = /^\d{1,2}(\-|\/|\.)\d{4}\1\d{1,2}$/;
        } else { // The year must be third
              reg1 = /^\d{1,2}(\-|\/|\.)\d{1,2}\1\d{2}$/;
              reg2 = /^\d{1,2}(\-|\/|\.)\d{1,2}\1\d{4}$/;
        }
        // If it doesn't conform to the right format (with either a 2 digit year
        // or 4 digit year), fail
        if(reg1.test(dateStr) === false && reg2.test(dateStr) === false) {
            return false;
        }
        // Split into 3 parts based on what the divider was
        let parts = dateStr.split(RegExp.$1);
        let mm, dd, yy;
        // Check to see if the 3 parts end up making a valid date
        if(format.substring(0, 1) === 'M'){
            mm = parts[0];
        } else if(format.substring(1, 2) === 'M'){
            mm = parts[1];
        } else {
            mm = parts[2];
        }
        if(format.substring(0, 1) === 'D'){
            dd = parts[0];
        } else if(format.substring(1, 2) === 'D'){
            dd = parts[1];
        } else {
            dd = parts[2];
        }
        if(format.substring(0, 1) === 'Y'){
            yy = parts[0];
        } else if(format.substring(1, 2) === 'Y'){
            yy = parts[1];
        } else {
            yy = parts[2];
        }
        if(parseInt(yy, 10) <= 50){
            yy = (parseInt(yy, 10) + 2000).toString();
        }
        if(parseInt(yy, 10) <= 99){
            yy = (parseInt(yy, 10) + 1900).toString();
        }
        let dt = new Date(
            parseInt(yy, 10), parseInt(mm, 10)-1, parseInt(dd, 10),
            0, 0, 0, 0);
        if(parseInt(dd, 10) != dt.getDate()){
            return false;
        }
        if(parseInt(mm, 10)-1 != dt.getMonth()){
            return false;
        }
        return true;
    },
    format(dateStr, formatStr) {
        if(!formatStr){
            formatStr = 'DMY';
        }
        if(!dateStr || dateStr === ''){
            return new Date(1001, 0, 1);
        }
        let oDate;
        let parts;

        switch(formatStr.toUpperCase()){
            case 'DDMMMYYYY':
                parts = dateStr.replace(/[- \/.]/g,' ').split(' ');
                oDate = new Date(y2kDate(parts[2]),mmm2mm(parts[1])-1,parts[0]);
            break;
            case 'DMY':
                /* jshint ignore:start */
                parts = dateStr.replace(
                    /^(0?[1-9]|[12][0-9]|3[01])([- \/.])(0?[1-9]|1[012])([- \/.])((\d\d)?\d\d)$/,'$1 $3 $5').split(' ');
                oDate = new Date(y2kDate(parts[2]),parts[1]-1,parts[0]);
                /* jshint ignore:end */
            break;
            case 'MDY':
                /* jshint ignore:start */
                parts = dateStr.replace(
                    /^(0?[1-9]|1[012])([- \/.])(0?[1-9]|[12][0-9]|3[01])([- \/.])((\d\d)?\d\d)$/,'$1 $3 $5').split(' ');
                oDate = new Date(y2kDate(parts[2]),parts[0]-1,parts[1]);
                /* jshint ignore:end */
            break;
            case 'YMD':
                /* jshint ignore:start */
                parts = dateStr.replace(/^((\d\d)?\d\d)([- \/.])(0?[1-9]|1[012])([- \/.])(0?[1-9]|[12][0-9]|3[01])$/,'$1 $4 $6').split(' ');
                oDate = new Date(y2kDate(parts[0]),parts[1]-1,parts[2]);
                /* jshint ignore:end */
            break;
            default: //in case format is not correct
                /* jshint ignore:start */
                parts = dateStr.replace(/^(0?[1-9]|[12][0-9]|3[01])([- \/.])(0?[1-9]|1[012])([- \/.])((\d\d)?\d\d)$/,'$1 $3 $5').split(' ');
                oDate = new Date(y2kDate(parts[2]),parts[1]-1,parts[0]);
                /* jshint ignore:end */
            break;
        }
        return oDate;
    }
};

function y2kDate(yr){
    if(yr === undefined){
        return 0;
    }
    if(yr.length>2){
        return yr;
    }
    let y;
    //>50 belong to 1900
    if(yr <= 99 && yr>50){
        y = '19' + yr;
    }
    //<50 belong to 2000
    if(yr<50 || yr === '00'){
        y = '20' + yr;
    }
    return y;
}

function mmm2mm(mmm){
    if(mmm === undefined){
        return 0;
    }
    let mondigit;
    let MONTH_NAMES = [
        'january','february','march','april','may','june','july',
        'august','september','october','november','december',
        'jan','feb','mar','apr','may','jun','jul','aug','sep','oct',
        'nov','dec'
    ];
    for(let m_i=0; m_i < MONTH_NAMES.length; m_i++){
            let month_name = MONTH_NAMES[m_i];
            if (mmm.toLowerCase() === month_name){
                mondigit = m_i+1;
                break;
            }
    }
    if(mondigit > 11 || mondigit < 23){
        mondigit = mondigit - 12;
    }
    if(mondigit < 1 || mondigit > 12){
        return 0;
    }
    return mondigit;
}
