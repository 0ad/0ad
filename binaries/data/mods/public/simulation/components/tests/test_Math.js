/**
 * Tests for consistent and correct math results
 */

 // +0 is different than -0, but standard equality won't test that
function isNegativeZero(z) { return z === 0 && 1/z === -Infinity; }
function isPositiveZero(z) { return z === 0 && 1/z === Infinity; }


// rounding
TS_ASSERT_EQUALS(0.1+0.2, 0.30000000000000004);
TS_ASSERT_EQUALS(0.1+0.7+0.3, 1.0999999999999999);

// cos
TS_ASSERT_EQUALS(Math.cos(Math.PI/2), 0);
TS_ASSERT_UNEVAL_EQUALS(Math.cos(NaN), NaN);
TS_ASSERT_EQUALS(Math.cos(0), 1);
TS_ASSERT_EQUALS(Math.cos(-0), 1);
TS_ASSERT_UNEVAL_EQUALS(Math.cos(Infinity), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.cos(-Infinity), NaN);

// sin
TS_ASSERT_EQUALS(Math.sin(Math.PI), 0);
TS_ASSERT_UNEVAL_EQUALS(Math.sin(NaN), NaN);
TS_ASSERT(isPositiveZero(Math.sin(0)));
// TS_ASSERT(isNegativeZero(Math.sin(-0))); TODO: doesn't match spec
TS_ASSERT_UNEVAL_EQUALS(Math.sin(Infinity), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.sin(-Infinity), NaN);
TS_ASSERT_EQUALS(Math.sin(1e-15), 7.771561172376096e-16);

// atan
TS_ASSERT_UNEVAL_EQUALS(Math.atan(NaN), NaN);
TS_ASSERT(isPositiveZero(Math.atan(0)));
TS_ASSERT(isNegativeZero(Math.atan(-0)));
TS_ASSERT_EQUALS(Math.atan(Infinity), Math.PI/2);
TS_ASSERT_EQUALS(Math.atan(-Infinity), -Math.PI/2);
TS_ASSERT_EQUALS(Math.atan(1e-310), 1.00000000003903e-310);
TS_ASSERT_EQUALS(Math.atan(100), 1.5607966601078411);

// atan2
TS_ASSERT_UNEVAL_EQUALS(Math.atan2(NaN, 1), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.atan2(1, NaN), NaN);
TS_ASSERT_EQUALS(Math.atan2(1, 0), Math.PI/2);
TS_ASSERT_EQUALS(Math.atan2(1, -0), Math.PI/2);
TS_ASSERT(isPositiveZero(Math.atan2(0, 1)));
TS_ASSERT(isPositiveZero(Math.atan2(0, 0)));
TS_ASSERT_EQUALS(Math.atan2(0, -0), Math.PI);
TS_ASSERT_EQUALS(Math.atan2(0, -1), Math.PI);
TS_ASSERT(isNegativeZero(Math.atan2(-0, 1)));
TS_ASSERT(isNegativeZero(Math.atan2(-0, 0)));
TS_ASSERT_EQUALS(Math.atan2(-0, -0), -Math.PI);
TS_ASSERT_EQUALS(Math.atan2(-0, -1), -Math.PI);
TS_ASSERT_EQUALS(Math.atan2(-1, 0), -Math.PI/2);
TS_ASSERT_EQUALS(Math.atan2(-1, -0), -Math.PI/2);
TS_ASSERT(isPositiveZero(Math.atan2(1.7e308, Infinity)));
TS_ASSERT_EQUALS(Math.atan2(1.7e308, -Infinity), Math.PI);
TS_ASSERT(isNegativeZero(Math.atan2(-1.7e308, Infinity)));
TS_ASSERT_EQUALS(Math.atan2(-1.7e308, -Infinity), -Math.PI);
TS_ASSERT_EQUALS(Math.atan2(Infinity, -1.7e308), Math.PI/2);
TS_ASSERT_EQUALS(Math.atan2(-Infinity, 1.7e308), -Math.PI/2);
TS_ASSERT_EQUALS(Math.atan2(Infinity, Infinity), Math.PI/4);
TS_ASSERT_EQUALS(Math.atan2(Infinity, -Infinity), 3*Math.PI/4);
TS_ASSERT_EQUALS(Math.atan2(-Infinity, Infinity), -Math.PI/4);
TS_ASSERT_EQUALS(Math.atan2(-Infinity, -Infinity), -3*Math.PI/4);
TS_ASSERT_EQUALS(Math.atan2(1e-310, 2), 5.0000000001954e-311);

// exp
TS_ASSERT_UNEVAL_EQUALS(Math.exp(NaN), NaN);
TS_ASSERT_EQUALS(Math.exp(0), 1);
TS_ASSERT_EQUALS(Math.exp(-0), 1);
TS_ASSERT_EQUALS(Math.exp(Infinity), Infinity);
TS_ASSERT(isPositiveZero(Math.exp(-Infinity)));
TS_ASSERT_EQUALS(Math.exp(10), 22026.465794806707);

// log
TS_ASSERT_UNEVAL_EQUALS(Math.log("NaN"), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.log(-1), NaN);
TS_ASSERT_EQUALS(Math.log(0), -Infinity);
TS_ASSERT_EQUALS(Math.log(-0), -Infinity);
TS_ASSERT(isPositiveZero(Math.log(1)));
TS_ASSERT_EQUALS(Math.log(Infinity), Infinity);
TS_ASSERT_EQUALS(Math.log(Math.E), 0.9999999999999991);
TS_ASSERT_EQUALS(Math.log(Math.E*Math.E*Math.E), 2.999999999999999);

// pow
TS_ASSERT_EQUALS(Math.pow(NaN, 0), 1);
TS_ASSERT_EQUALS(Math.pow(NaN, -0), 1);
TS_ASSERT_UNEVAL_EQUALS(Math.pow(NaN, 100), NaN);
TS_ASSERT_EQUALS(Math.pow(1.7e308, Infinity), Infinity);
TS_ASSERT_EQUALS(Math.pow(-1.7e308, Infinity), Infinity);
TS_ASSERT(isPositiveZero(Math.pow(1.7e308, -Infinity)));
TS_ASSERT(isPositiveZero(Math.pow(-1.7e308, -Infinity)));
TS_ASSERT_UNEVAL_EQUALS(Math.pow(1, Infinity), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.pow(-1, Infinity), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.pow(1, -Infinity), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.pow(-1, -Infinity), NaN);
TS_ASSERT(isPositiveZero(Math.pow(1e-310, Infinity)));
TS_ASSERT(isPositiveZero(Math.pow(-1e-310, Infinity)));
TS_ASSERT_EQUALS(Math.pow(1e-310, -Infinity), Infinity);
TS_ASSERT_EQUALS(Math.pow(-1e-310, -Infinity), Infinity);
TS_ASSERT_EQUALS(Math.pow(Infinity, 1e-310), Infinity);
TS_ASSERT(isPositiveZero(Math.pow(Infinity, -1e-310)));
TS_ASSERT_EQUALS(Math.pow(-Infinity, 101), -Infinity);
TS_ASSERT_EQUALS(Math.pow(-Infinity, 1.7e308), Infinity);
TS_ASSERT(isNegativeZero(Math.pow(-Infinity, -101)));
TS_ASSERT(isPositiveZero(Math.pow(-Infinity, -1.7e308)));
TS_ASSERT(isPositiveZero(Math.pow(0, 1e-310)));
TS_ASSERT_EQUALS(Math.pow(0, -1e-310), Infinity);
TS_ASSERT(isNegativeZero(Math.pow(-0, 101)));
TS_ASSERT(isPositiveZero(Math.pow(-0, 1e-310)));
TS_ASSERT_EQUALS(Math.pow(-0, -101), -Infinity);
TS_ASSERT_EQUALS(Math.pow(-0, -1e-310), Infinity);
TS_ASSERT_UNEVAL_EQUALS(Math.pow(-1.7e308, 1e-310), NaN);
TS_ASSERT_EQUALS(Math.pow(Math.PI, -100), 1.9275814160560185e-50);

// sqrt
TS_ASSERT_UNEVAL_EQUALS(Math.sqrt(NaN), NaN);
TS_ASSERT_UNEVAL_EQUALS(Math.sqrt(-1e-323), NaN);
TS_ASSERT(isPositiveZero(Math.sqrt(0)));
TS_ASSERT(isNegativeZero(Math.sqrt(-0)));
TS_ASSERT_EQUALS(Math.sqrt(Infinity), Infinity);
TS_ASSERT_EQUALS(Math.sqrt(1e-323), 3.1434555694052576e-162);

// square
TS_ASSERT_UNEVAL_EQUALS(Math.square(NaN), NaN);
TS_ASSERT(isPositiveZero(Math.square(0)));
TS_ASSERT(isPositiveZero(Math.square(-0)));
TS_ASSERT_EQUALS(Math.square(Infinity), Infinity);
TS_ASSERT_EQUALS(Math.square(1.772979291871526e-81),3.143455569405258e-162);
TS_ASSERT_EQUALS(Math.square(1e+155), Infinity);
TS_ASSERT_UNEVAL_EQUALS(Math.square(1), 1);
TS_ASSERT_UNEVAL_EQUALS(Math.square(20), 400);
TS_ASSERT_UNEVAL_EQUALS(Math.square(300), 90000);
TS_ASSERT_UNEVAL_EQUALS(Math.square(4000), 16000000);
TS_ASSERT_UNEVAL_EQUALS(Math.square(50000), 2500000000);
TS_ASSERT_UNEVAL_EQUALS(Math.square(-3), 9);
TS_ASSERT_UNEVAL_EQUALS(Math.square(-8), 64);
TS_ASSERT_UNEVAL_EQUALS(Math.square(0.123), 0.015129);

// euclid distance
TS_ASSERT_UNEVAL_EQUALS(Math.euclidDistance2D(0, 0, 3, 4), 5);
TS_ASSERT_UNEVAL_EQUALS(Math.euclidDistance2D(-4, -4, -5, -4), 1);
TS_ASSERT_UNEVAL_EQUALS(Math.euclidDistance2D(1e+140, 1e+140, 0, 0), 1.414213562373095e+140);
TS_ASSERT_UNEVAL_EQUALS(Math.euclidDistance3D(0, 0, 0, 20, 48, 165), 173);
