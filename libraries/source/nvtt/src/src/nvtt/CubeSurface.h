// Copyright (c) 2009-2011 Ignacio Castano <castano@gmail.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NVTT_CUBEIMAGE_H
#define NVTT_CUBEIMAGE_H

#include "nvtt.h"
#include "Surface.h"

#include "nvimage/FloatImage.h"

#include "nvmath/Vector.h"

#include "nvcore/RefCounted.h"
#include "nvcore/Ptr.h"
#include "nvcore/Array.h"


namespace nvtt
{
    struct TexelTable {
        TexelTable(uint edgeLength);

        float solidAngle(uint f, uint x, uint y) const;
        const nv::Vector3 & direction(uint f, uint x, uint y) const;

        uint size;
        nv::Array<float> solidAngleArray;
        nv::Array<nv::Vector3> directionArray;
    };


    struct CubeSurface::Private : public nv::RefCounted
    {
        void operator=(const Private &);
    public:
        Private()
        {
            nvDebugCheck( refCount() == 0 );

            edgeLength = 0;
            texelTable = NULL;
        }
        Private(const Private & p) : RefCounted() // Copy ctor. inits refcount to 0.
        {
            nvDebugCheck( refCount() == 0 );

            edgeLength = p.edgeLength;
            for (uint i = 0; i < 6; i++) {
                face[i] = p.face[i];
            }
            texelTable = NULL; // @@ Transfer tables. Needs refcounting?
        }
        ~Private()
        {
            delete texelTable;
        }

        void allocate(uint edgeLength)
        {
            this->edgeLength = edgeLength;
            for (uint i = 0; i < 6; i++) {
                face[i].detach();
                face[i].m->image = new nv::FloatImage;
                face[i].m->image->allocate(4, edgeLength, edgeLength, 1);
            }
        }

        void allocateTexelTable()
        {
            if (texelTable == NULL) {
                texelTable = new TexelTable(edgeLength);
            }
        }

        // Filtering helpers:
        nv::Vector3 applyAngularFilter(const nv::Vector3 & dir, float coneAngle, float * filterTable, int tableSize);
        nv::Vector3 applyCosinePowerFilter(const nv::Vector3 & dir, float coneAngle, float cosinePower);

        nv::Vector3 sample(const nv::Vector3 & dir);

        uint edgeLength;
        Surface face[6];
        TexelTable * texelTable;
    };

} // nvtt namespace


#endif // NVTT_CUBEIMAGE_H
