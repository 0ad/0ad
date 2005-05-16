const SIZE = 64;

init(SIZE, "grass1_a", 1.5);

for(var x=0; x<SIZE; x++) {
    for(var y=0; y<SIZE; y++) {
        setTerrain(x,y, chooseRand("grass1_a","dirta","snow","road1"));
    }
}