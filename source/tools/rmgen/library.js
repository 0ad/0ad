function println(x) {
    print(x);
    print("\n");
}

function chooseRand() {
    if(arguments.length==0) {
        error("chooseRand: requires at least 1 argument");
    }
    var ar = (arguments.length==1 ? arguments[0] : arguments);
    return ar[randInt(ar.length)];
}