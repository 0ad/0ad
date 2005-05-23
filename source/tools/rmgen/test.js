const SIZE = 48;

init(SIZE, "grass1_a", 0);

var placer = new RectPlacer(0,0,3,4);
placer.y2 = 6;
createArea(placer, 0, 0);

for(var x=0; x<SIZE; x++) {
    for(var y=0; y<SIZE; y++) {
        var t = chooseRand("grass1_a", "grass2", "grass1_dense", "grass_dry_25", "forrestfloor");
        setTerrain(x, y, t);
        setHeight(x, y, randFloat()*3);
        if(t=="forrestfloor") {
            addEntity("wrld_flora_oak", 3, x+0.5, y+0.5, randFloat()*2.0*Math.PI);
        }
    }
}
