//TODO: Move to some library file?

//const LOAD_NOTHING = 0; 
const LOAD_TERRAIN = 1
const LOAD_INTERACTIVES = 2;
const LOAD_NON_INTERACTIVES = 4;

const LOAD_ALL = LOAD_TERRAIN | LOAD_INTERACTIVES | LOAD_NON_INTERACTIVES;


initFromScenario("mediterannean", LOAD_TERRAIN);

