//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 07/09/2002 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_region.c
//
// Description: Creates an image region.
//
//-----------------------------------------------------------------------------


#include "ilu_region.h"


ILpointi	*RegionPointsi = NULL;
ILpointf	*RegionPointsf = NULL;
ILuint		PointNum = 0;
ILubyte		*iRegionMask = NULL;

ILvoid ILAPIENTRY iluRegionfv(ILpointf *Points, ILuint n)
{
	if (Points == NULL || n == 0) {
		ifree(RegionPointsi);
		ifree(RegionPointsf);
		RegionPointsf = NULL;
		PointNum = 0;
		return;
	}
	if (n < 3) {
		ilSetError(ILU_INVALID_PARAM);
		return;
	}
	ifree(RegionPointsi);
	ifree(RegionPointsf);
	RegionPointsf = (ILpointf*)ialloc(sizeof(ILpointf) * n);
	if (RegionPointsf == NULL)
		return;
	memcpy(RegionPointsf, Points, sizeof(ILpointi) * n);
	PointNum = n;
	return;
}


ILvoid ILAPIENTRY iluRegioniv(ILpointi *Points, ILuint n)
{
	if (Points == NULL || n == 0) {
		ifree(RegionPointsi);
		ifree(RegionPointsf);
		RegionPointsi = NULL;
		PointNum = 0;
		return;
	}
	if (n < 3) {
		ilSetError(ILU_INVALID_PARAM);
		return;
	}
	ifree(RegionPointsi);
	ifree(RegionPointsf);
	RegionPointsi = (ILpointi*)ialloc(sizeof(ILpointi) * n);
	if (RegionPointsi == NULL)
		return;
	memcpy(RegionPointsi, Points, sizeof(ILpointi) * n);
	PointNum = n;
	return;
}


// Inserts edge into list in order of increasing xIntersect field.
void InsertEdge(Edge *list, Edge *edge)
{
	Edge *p, *q = list;

	p = q->next;
	while (p != NULL) {
		if (edge->xIntersect < p->xIntersect) {
			p = NULL;
		}
		else {
			q = p;
			p = p->next;
		}
	}
	edge->next = q->next;
	q->next = edge;
}


// For an index, return y-coordinate of next nonhorizontal line
ILint yNext(ILint k, ILint cnt, ILpointi *pts)
{
	ILint j;

	if ((k+1) > (cnt-1))
		j = 0;
	else
		j = k + 1;

	while (pts[k].y == pts[j].y) {
		if ((j+1) > (cnt-1))
			j = 0;
		else
			j++;
	}

	return pts[j].y;
}


// Store lower-y coordinate and inverse slope for each edge.  Adjust
//	and store upper-y coordinate for edges that are the lower member
//	of a monotonically increasing or decreasing pair of edges
void MakeEdgeRec(ILpointi lower, ILpointi upper, ILint yComp, Edge *edge, Edge *edges[])
{
	edge->dxPerScan = (ILfloat)(upper.x - lower.x) / (upper.y - lower.y);
	edge->xIntersect = (ILfloat)lower.x;
	if (upper.y < yComp)
		edge->yUpper = upper.y - 1;
	else
		edge->yUpper = upper.y;

	InsertEdge(edges[lower.y], edge);
}


void BuildEdgeList(ILuint cnt, ILpointi *pts, Edge **edges)
{
	Edge *edge;
	ILpointi v1, v2;
	ILuint i;
	ILint yPrev = pts[cnt - 2].y;

	v1.x = pts[cnt-1].x;
	v1.y = pts[cnt-1].y;

	for (i = 0; i < cnt; i++) {
		v2 = pts[i];
		if (v1.y != v2.y) {			// nonhorizontal line
			edge = (Edge*)ialloc(sizeof(Edge));
			if (v1.y < v2.y) {		// up-going edge
				MakeEdgeRec(v1, v2, yNext(i, cnt, pts), edge, edges);
			}
			else {					// down-going edge
				MakeEdgeRec(v2, v1, yPrev, edge, edges);
			}
		}
		yPrev = v1.y;
		v1 = v2;
	}
}


void BuildActiveList(ILint scan, Edge *active, Edge *edges[])
{
	Edge *p, *q;

	p = edges[scan]->next;
	while (p) {
		q = p->next;
		InsertEdge(active, p);
		p = q;
	}
}


#define iRegionSetPixel(x,y) (iRegionMask[y * iluCurImage->Width + x] = 1 )


void FillScan(ILint scan, Edge *active)
{
	Edge *p1, *p2;
	ILint i;

	p1 = active->next;
	while (p1) {
		p2 = p1->next;
		for (i = (ILuint)p1->xIntersect; i < p2->xIntersect; i++) {
			iRegionSetPixel((ILuint)i, scan);
		}
		p1 = p2->next;
	}
}


void DeleteAfter(Edge *q)
{
	Edge *p = q->next;
	q->next = p->next;
	free(p);
}


// Delete completed edges.  Update 'xIntersect' field for others
void UpdateActiveList(ILint scan, Edge *active)
{
	Edge *q = active, *p = active->next;

	while (p) {
		if (scan >= p->yUpper) {
			p = p->next;
			DeleteAfter(q);
		}
		else {
			p->xIntersect = p->xIntersect + p->dxPerScan;
			q = p;
			p = p->next;
		}
	}
}


void ResortActiveList(Edge *active)
{
	Edge *q, *p = active->next;

	active->next = NULL;
	while (p) {
		q = p->next;
		InsertEdge(active, p);
		p = q;
	}
}


ILubyte *iScanFill()
{
	Edge	**edges = NULL, *active = NULL/*, *temp*/;
	ILuint	i, scan;

	iRegionMask = NULL;

	if ((RegionPointsi == NULL && RegionPointsf == NULL) || PointNum == 0)
		return NULL;

	if (RegionPointsf) {
		RegionPointsi = (ILpointi*)ialloc(sizeof(ILpointi) * PointNum);
		if (RegionPointsi == NULL)
			goto error;
	}

	for (i = 0; i < PointNum; i++) {
		if (RegionPointsf) {
			RegionPointsi[i].x = (ILuint)(iluCurImage->Width * RegionPointsf[i].x);
			RegionPointsi[i].y = (ILuint)(iluCurImage->Height * RegionPointsf[i].y);
		}
		if (RegionPointsi[i].x >= (ILint)iluCurImage->Width || RegionPointsi[i].y >= (ILint)iluCurImage->Height)
			goto error;
	}

	edges = (Edge**)ialloc(sizeof(Edge*) * iluCurImage->Height);
	iRegionMask = (ILubyte*)ialloc(iluCurImage->Width * iluCurImage->Height * iluCurImage->Depth);
	if (edges == NULL || iRegionMask == NULL)
		goto error;
	imemclear(iRegionMask, iluCurImage->Width * iluCurImage->Height * iluCurImage->Depth);

	for (i = 0; i < iluCurImage->Height; i++) {
		edges[i] = (Edge*)ialloc(sizeof(Edge));
		edges[i]->next = NULL;
	}
	BuildEdgeList(PointNum, RegionPointsi, edges);
	active = (Edge*)ialloc(sizeof(Edge));
	active->next = NULL;

	for (scan = 0; scan < iluCurImage->Height; scan++) {
		BuildActiveList(scan, active, edges);
		if (active->next) {
			FillScan(scan, active);
			UpdateActiveList(scan, active);
			ResortActiveList(active);
		}
	}

	// Free edge records that have been allocated.
	/*for (i = 0; i < iluCurImage->Height; i++) {
		while (edges[i]) {
			temp = edges[i]->next;
			ifree(edges[i]);
			edges[i] = temp;
		}
	}*/

	ifree(edges);

	if (RegionPointsf) {
		ifree(RegionPointsi);
		RegionPointsi = NULL;
	}

	return iRegionMask;

error:
	if (RegionPointsf) {
		ifree(RegionPointsi);
		RegionPointsi = NULL;
	}

	// Free edge records that have been allocated.

	ifree(edges);
	ifree(iRegionMask);
	return NULL;
}

