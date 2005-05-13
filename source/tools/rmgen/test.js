print("Hello world!\n");

init(32, "grass1_a", 1.5);

print(getHeight(0,0) + "\n");

for(x=0; x<3; x++) {
    for(y=0; y<20; y++) {
        setTerrain(x, y, "dirta");
        setHeight(x, y, 4.5);
    }
}

print(getHeight(0,0) + "\n");
