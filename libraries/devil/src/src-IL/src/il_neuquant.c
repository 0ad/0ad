/* NeuQuant Neural-Net Quantization Algorithm
 * ------------------------------------------
 *
 * Copyright (c) 1994 Anthony Dekker
 *
 * NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994.
 * See "Kohonen neural networks for optimal colour quantization"
 * in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
 * for a discussion of the algorithm.
 * See also  http://www.acm.org/~dekker/NEUQUANT.HTML
 *
 * Any party obtaining a copy of these files from the author, directly or
 * indirectly, is granted, free of charge, a full and unrestricted irrevocable,
 * world-wide, paid up, royalty-free, nonexclusive right and license to deal
 * in this software and documentation files (the "Software"), including without
 * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so, with the only requirement being
 * that this copyright notice remain intact.
 */

//-----------------------------------------------------------------------------
//
// ImageLib Sources
// by Denton Woods
// Last modified: 02/02/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_neuquant.c
//
// Description: Heavily modified by Denton Woods.
//
//-----------------------------------------------------------------------------

#include "il_internal.h"


// Function definitions
ILvoid	initnet(ILubyte *thepic, ILint len, ILint sample);
ILvoid	unbiasnet();
ILvoid	inxbuild();
ILubyte	inxsearch(ILint b, ILint g, ILint r);
ILvoid	learn();

// four primes near 500 - assume no image has a length so large
// that it is divisible by all four primes
#define prime1			499
#define prime2			491
#define prime3			487
#define prime4			503

#define minpicturebytes	(3*prime4)			// minimum size for input image


// Network Definitions
// -------------------
   
#define netsize			256					// number of colours used
#define maxnetpos		(netsizethink-1)
#define netbiasshift	4					// bias for colour values
#define ncycles			100					// no. of learning cycles

// defs for freq and bias
#define intbiasshift	16					// bias for fractions
#define intbias			(((ILint) 1)<<intbiasshift)
#define gammashift		10					// gamma = 1024
#define gamma			(((ILint) 1)<<gammashift)
#define betashift		10
#define beta			(intbias>>betashift)// beta = 1/1024
#define betagamma		(intbias<<(gammashift-betashift))

// defs for decreasing radius factor
#define initrad			(netsize>>3)		// for 256 cols, radius starts
#define radiusbiasshift	6					// at 32.0 biased by 6 bits
#define radiusbias		(((ILint) 1)<<radiusbiasshift)
#define initradius		(initrad*radiusbias)	// and decreases by a
#define radiusdec		30						// factor of 1/30 each cycle

// defs for decreasing alpha factor
#define alphabiasshift	10						// alpha starts at 1.0
#define initalpha		(((ILint) 1)<<alphabiasshift)
ILint	alphadec;								// biased by 10 bits

// radbias and alpharadbias used for radpower calculation
#define radbiasshift	8
#define radbias			(((ILint) 1)<<radbiasshift)
#define alpharadbshift	(alphabiasshift+radbiasshift)
#define alpharadbias	(((ILint) 1)<<alpharadbshift)


// Types and Global Variables
// --------------------------
   
unsigned char	*thepicture;			// the input image itself
int				lengthcount;			// lengthcount = H*W*3
int				samplefac;				// sampling factor 1..30
typedef int		pixel[4];				// BGRc
static pixel	network[netsize];		// the network itself
int				netindex[256];			// for network lookup - really 256
int				bias [netsize];			// bias and freq arrays for learning
int				freq [netsize];
int				radpower[initrad];		// radpower for precomputation

int netsizethink; // number of colors we want to reduce to, 2-256

// Initialise network in range (0,0,0) to (255,255,255) and set parameters
// -----------------------------------------------------------------------

ILvoid initnet(ILubyte *thepic, ILint len, ILint sample)	
{
	ILint i;
	ILint *p;
	
	thepicture = thepic;
	lengthcount = len;
	samplefac = sample;
	
	for (i=0; i<netsizethink; i++) {
		p = network[i];
		p[0] = p[1] = p[2] = (i << (netbiasshift+8))/netsize;
		freq[i] = intbias/netsizethink;	// 1/netsize
		bias[i] = 0;
	}
	return;
}

	
// Unbias network to give byte values 0..255 and record position i to prepare for sort
// -----------------------------------------------------------------------------------

ILvoid unbiasnet()
{
	ILint i,j;

	for (i=0; i<netsizethink; i++) {
		for (j=0; j<3; j++)
			network[i][j] >>= netbiasshift;
		network[i][3] = i;			// record colour no
	}
	return;
}


// Insertion sort of network and building of netindex[0..255] (to do after unbias)
// -------------------------------------------------------------------------------

ILvoid inxbuild()
{
	ILint i,j,smallpos,smallval;
	ILint *p,*q;
	ILint previouscol,startpos;

	previouscol = 0;
	startpos = 0;
	for (i=0; i<netsizethink; i++) {
		p = network[i];
		smallpos = i;
		smallval = p[1];			// index on g
		// find smallest in i..netsize-1
		for (j=i+1; j<netsizethink; j++) {
			q = network[j];
			if (q[1] < smallval) {	// index on g
				smallpos = j;
				smallval = q[1];	// index on g
			}
		}
		q = network[smallpos];
		// swap p (i) and q (smallpos) entries
		if (i != smallpos) {
			j = q[0];   q[0] = p[0];   p[0] = j;
			j = q[1];   q[1] = p[1];   p[1] = j;
			j = q[2];   q[2] = p[2];   p[2] = j;
			j = q[3];   q[3] = p[3];   p[3] = j;
		}
		// smallval entry is now in position i
		if (smallval != previouscol) {
			netindex[previouscol] = (startpos+i)>>1;
			for (j=previouscol+1; j<smallval; j++) netindex[j] = i;
			previouscol = smallval;
			startpos = i;
		}
	}
	netindex[previouscol] = (startpos+maxnetpos)>>1;
	for (j=previouscol+1; j<256; j++) netindex[j] = maxnetpos; // really 256
	return;
}


// Search for BGR values 0..255 (after net is unbiased) and return colour index
// ----------------------------------------------------------------------------

ILubyte inxsearch(ILint b, ILint g, ILint r)
{
	ILint i,j,dist,a,bestd;
	ILint *p;
	ILint best;

	bestd = 1000;		// biggest possible dist is 256*3
	best = -1;
	i = netindex[g];	// index on g
	j = i-1;			// start at netindex[g] and work outwards

	while ((i<netsizethink) || (j>=0)) {
		if (i<netsizethink) {
			p = network[i];
			dist = p[1] - g;		// inx key
			if (dist >= bestd) i = netsizethink;	// stop iter
			else {
				i++;
				if (dist<0) dist = -dist;
				a = p[0] - b;   if (a<0) a = -a;
				dist += a;
				if (dist<bestd) {
					a = p[2] - r;   if (a<0) a = -a;
					dist += a;
					if (dist<bestd) {bestd=dist; best=p[3];}
				}
			}
		}
		if (j>=0) {
			p = network[j];
			dist = g - p[1]; // inx key - reverse dif
			if (dist >= bestd) j = -1; // stop iter
			else {
				j--;
				if (dist<0) dist = -dist;
				a = p[0] - b;   if (a<0) a = -a;
				dist += a;
				if (dist<bestd) {
					a = p[2] - r;   if (a<0) a = -a;
					dist += a;
					if (dist<bestd) {bestd=dist; best=p[3];}
				}
			}
		}
	}
	return (ILubyte)best;
}


// Search for biased BGR values
// ----------------------------

ILint contest(ILint b, ILint g, ILint r)
{
	// finds closest neuron (min dist) and updates freq
	// finds best neuron (min dist-bias) and returns position
	// for frequently chosen neurons, freq[i] is high and bias[i] is negative
	// bias[i] = gamma*((1/netsize)-freq[i])

	ILint i,dist,a,biasdist,betafreq;
	ILint bestpos,bestbiaspos,bestd,bestbiasd;
	ILint *p,*f, *n;

	bestd = ~(((ILint) 1)<<31);
	bestbiasd = bestd;
	bestpos = -1;
	bestbiaspos = bestpos;
	p = bias;
	f = freq;

	for (i=0; i<netsizethink; i++) {
		n = network[i];
		dist = n[0] - b;   if (dist<0) dist = -dist;
		a = n[1] - g;   if (a<0) a = -a;
		dist += a;
		a = n[2] - r;   if (a<0) a = -a;
		dist += a;
		if (dist<bestd) {bestd=dist; bestpos=i;}
		biasdist = dist - ((*p)>>(intbiasshift-netbiasshift));
		if (biasdist<bestbiasd) {bestbiasd=biasdist; bestbiaspos=i;}
		betafreq = (*f >> betashift);
		*f++ -= betafreq;
		*p++ += (betafreq<<gammashift);
	}
	freq[bestpos] += beta;
	bias[bestpos] -= betagamma;
	return(bestbiaspos);
}


// Move neuron i towards biased (b,g,r) by factor alpha
// ----------------------------------------------------

ILvoid altersingle(ILint alpha, ILint i, ILint b, ILint g, ILint r)
{
	ILint *n;

	n = network[i];				// alter hit neuron
	*n -= (alpha*(*n - b)) / initalpha;
	n++;
	*n -= (alpha*(*n - g)) / initalpha;
	n++;
	*n -= (alpha*(*n - r)) / initalpha;
	return;
}


// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
// ---------------------------------------------------------------------------------

ILvoid alterneigh(ILint rad, ILint i, ILint b, ILint g, ILint r)
{
	ILint j,k,lo,hi,a;
	ILint *p, *q;

	lo = i-rad;   if (lo<-1) lo=-1;
	hi = i+rad;   if (hi>netsizethink) hi=netsizethink;

	j = i+1;
	k = i-1;
	q = radpower;
	while ((j<hi) || (k>lo)) {
		a = (*(++q));
		if (j<hi) {
			p = network[j];
			*p -= (a*(*p - b)) / alpharadbias;
			p++;
			*p -= (a*(*p - g)) / alpharadbias;
			p++;
			*p -= (a*(*p - r)) / alpharadbias;
			j++;
		}
		if (k>lo) {
			p = network[k];
			*p -= (a*(*p - b)) / alpharadbias;
			p++;
			*p -= (a*(*p - g)) / alpharadbias;
			p++;
			*p -= (a*(*p - r)) / alpharadbias;
			k--;
		}
	}
	return;
}


// Main Learning Loop
// ------------------

ILvoid learn()
{
	ILint i,j,b,g,r;
	ILint radius,rad,alpha,step,delta,samplepixels;
	ILubyte *p;
	ILubyte *lim;

	alphadec = 30 + ((samplefac-1)/3);
	p = thepicture;
	lim = thepicture + lengthcount;
	samplepixels = lengthcount/(3*samplefac);
	delta = samplepixels/ncycles;
	alpha = initalpha;
	radius = initradius;
	
	rad = radius >> radiusbiasshift;
	if (rad <= 1) rad = 0;
	for (i=0; i<rad; i++) 
		radpower[i] = alpha*(((rad*rad - i*i)*radbias)/(rad*rad));
	
	// beginning 1D learning: initial radius=rad

	if ((lengthcount%prime1) != 0) step = 3*prime1;
	else {
		if ((lengthcount%prime2) !=0) step = 3*prime2;
		else {
			if ((lengthcount%prime3) !=0) step = 3*prime3;
			else step = 3*prime4;
		}
	}
	
	i = 0;
	while (i < samplepixels) {
		b = p[0] << netbiasshift;
		g = p[1] << netbiasshift;
		r = p[2] << netbiasshift;
		j = contest(b,g,r);

		altersingle(alpha,j,b,g,r);
		if (rad) alterneigh(rad,j,b,g,r);   // alter neighbours

		p += step;
		if (p >= lim) p -= lengthcount;
	
		i++;
		if (i%delta == 0) {	
			alpha -= alpha / alphadec;
			radius -= radius / radiusdec;
			rad = radius >> radiusbiasshift;
			if (rad <= 1) rad = 0;
			for (j=0; j<rad; j++) 
				radpower[j] = alpha*(((rad*rad - j*j)*radbias)/(rad*rad));
		}
	}
	// finished 1D learning: final alpha=alpha/initalpha;
	return;
}


ILimage *iNeuQuant(ILimage *Image)
{
	ILimage	*TempImage, *NewImage;
	ILuint	sample, i, j;

	netsizethink=iGetInt(IL_MAX_QUANT_INDEXS);

	NewImage = iCurImage;
	iCurImage = Image;
	TempImage = iConvertImage(iCurImage, IL_BGR, IL_UNSIGNED_BYTE);
	iCurImage = NewImage;
	sample = ilGetInteger(IL_NEU_QUANT_SAMPLE);

	if (TempImage == NULL)
		return NULL;

	initnet(TempImage->Data, TempImage->SizeOfData, sample);
	learn();
	unbiasnet();

	NewImage = (ILimage*)icalloc(sizeof(ILimage), 1);
	if (NewImage == NULL) {
		ilCloseImage(TempImage);
		return NULL;
	}
	NewImage->Data = (ILubyte*)ialloc(TempImage->SizeOfData / 3);
	if (NewImage->Data == NULL) {
		ilCloseImage(TempImage);
		ifree(NewImage);
		return NULL;
	}
	ilCopyImageAttr(NewImage, Image);
	NewImage->Bpp = 1;
	NewImage->Bps = Image->Width;
	NewImage->SizeOfPlane = NewImage->Bps * Image->Height;
	NewImage->SizeOfData = NewImage->SizeOfPlane;
	NewImage->Format = IL_COLOUR_INDEX;
	NewImage->Type = IL_UNSIGNED_BYTE;

	NewImage->Pal.PalSize = netsizethink * 3;
	NewImage->Pal.PalType = IL_PAL_BGR24;
	NewImage->Pal.Palette = (ILubyte*)ialloc(256*3);
	if (NewImage->Pal.Palette == NULL) {
		ilCloseImage(TempImage);
		ilCloseImage(NewImage);
		return NULL;
	}

	for (i = 0, j = 0; i < (unsigned)netsizethink; i++, j += 3) {
		NewImage->Pal.Palette[j  ] = network[i][0];
		NewImage->Pal.Palette[j+1] = network[i][1];
		NewImage->Pal.Palette[j+2] = network[i][2];
	}

	inxbuild();
	for (i = 0, j = 0; j < TempImage->SizeOfData; i++, j += 3) {
		NewImage->Data[i] = inxsearch(
			TempImage->Data[j], TempImage->Data[j+1], TempImage->Data[j+2]);
	}

	ilCloseImage(TempImage);

	return NewImage;
}
