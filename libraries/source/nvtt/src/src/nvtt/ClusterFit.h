/* -----------------------------------------------------------------------------

    Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
    Copyright (c) 2006 Ignacio Castano                      icastano@nvidia.com

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to	deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#ifndef NVTT_CLUSTERFIT_H
#define NVTT_CLUSTERFIT_H

#include "nvmath/SimdVector.h"
#include "nvmath/Vector.h"
#include "nvcore/Memory.h"

// Use SIMD version if altivec or SSE are available.
#define NVTT_USE_SIMD (NV_USE_ALTIVEC || NV_USE_SSE)
//#define NVTT_USE_SIMD 0

namespace nv {

    struct ColorSet;

    class ClusterFit
    {
    public:
        ClusterFit();

        //void setColorSet(const ColorSet * set);
        void setColorSet(const Vector3 * colors, const float * weights, int count);

        void setColorWeights(const Vector4 & w);
        float bestError() const;

        bool compress3(Vector3 * start, Vector3 * end);
        bool compress4(Vector3 * start, Vector3 * end);

    private:

        uint m_count;

        // IC: Color and weight arrays are larger than necessary to avoid compiler warning.

    #if NVTT_USE_SIMD
        NV_ALIGN_16 SimdVector m_weighted[17];  // color | weight
        SimdVector m_metric;                    // vec3
        SimdVector m_metricSqr;                 // vec3
        SimdVector m_xxsum;                     // color | weight
        SimdVector m_xsum;                      // color | weight (wsum)
        SimdVector m_besterror;                 // scalar
    #else
        Vector3 m_weighted[17];
        float m_weights[17];
        Vector3 m_metric;
        Vector3 m_metricSqr;
        Vector3 m_xxsum;
        Vector3 m_xsum;
        float m_wsum;
        float m_besterror;
    #endif
    };

} // nv namespace

#endif // NVTT_CLUSTERFIT_H
