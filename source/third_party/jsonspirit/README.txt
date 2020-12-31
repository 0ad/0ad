This separate JSON library is used for Atlas to avoid the SpiderMonkey dependency.
SpiderMonkey is a fully featured JS engine and even though we already use it for the main engine, it's too heavy-weight to use it in Atlas.
The SpiderMonkey API also changes frequently and we hope that the JSON parsing code needs less changes when we use this separate library.

Get the library from here:
http://www.codeproject.com/Articles/20027/JSON-Spirit-A-C-JSON-Parser-Generator-Implemented

The currently used version was released on the 10 of May 2014.

Search for this comment in json_spirit_value.h and uncomment the lines we don't need:
// comment out the value types you don't need to reduce build times and intermediate file sizes
