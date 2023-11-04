/*
    ==============================================================================
    This file is part of Obxd synthesizer.

    Copyright © 2013-2014 Filatov Vadim

    Contact author via email :
    justdat_@_e1.ru

    This file may be licensed under the terms of of the
    GNU General Public License Version 2 (the ``GPL'').

    Software distributed under the License is distributed
    on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
    express or implied. See the GPL for the specific language
    governing rights and limitations.

    You should have received a copy of the GPL along with this
    program. If not, go to http://www.gnu.org/licenses/gpl.html
    or write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
    ==============================================================================
 */
#pragma once

#include "BlepData.h"

class TriangleOsc
{
    static constexpr int N = Samples * 2;
    static constexpr int MASK = N - 1;

public:
    void setDecimation()
    {
        blepPTR = blepd2;
        blampPTR = blampd2;
    }

    void removeDecimation()
    {
        blepPTR = blep;
        blampPTR = blamp;
    }

    float tick(float x)
    {
        float mix = x < 0.5 ? 2 * x - 0.5 : 1.5 - 2 * x;
        return delay.tick(mix) - getNextBlep();
    }

    void processMaster(float x, float delta)
    {
        if (x >= 1.0)
        {
            x -= 1.0;
            applyBLAMP(x / delta, -4 * Samples * delta);
        }
        if (x >= 0.5 && x - delta < 0.5)
        {
            applyBLAMP((x - 0.5) / delta, 4 * Samples * delta);
        }
        if (x >= 1.0)
        {
            x -= 1.0;
            applyBLAMP(x / delta, -4 * Samples * delta);
        }
    }

    void processSlave(float x, float delta, bool hardSyncReset, float hardSyncFrac)
    {
        bool hspass = true;
        if (x >= 1.0)
        {
            x -= 1.0;
            if (! hardSyncReset || x / delta > hardSyncFrac) //de morgan processed equation
            {
                applyBLAMP(x / delta, -4 * Samples * delta);
            }
            else
            {
                x += 1;
                hspass = false;
            }
        }
        if (x >= 0.5 && x - delta < 0.5 && hspass)
        {
            float frac = (x - 0.5) / delta;
            if (! hardSyncReset || frac > hardSyncFrac) //de morgan processed equation
            {
                applyBLAMP(frac, 4 * Samples * delta);
            }
        }
        if (x >= 1.0 && hspass)
        {
            x -= 1.0;
            if (! hardSyncReset || x / delta > hardSyncFrac) //de morgan processed equation
            {
                applyBLAMP(x / delta, -4 * Samples * delta);
            }
            else
            {
                //if transition did not occur
                x += 1;
            }
        }

        if (hardSyncReset)
        {
            float fracMaster = (delta * hardSyncFrac);
            float trans = (x - fracMaster);
            float mix = trans < 0.5 ? 2 * trans - 0.5 : 1.5 - 2 * trans;
            if (trans > 0.5)
                applyBLAMP(hardSyncFrac, -4 * Samples * delta);
            applyBLEP(hardSyncFrac, mix + 0.5);
        }
    }

private:
    void applyBLEP(const float offset, const float scale)
    {
        int lpIn = B_OVERSAMPLING * offset;
        const float frac = offset * B_OVERSAMPLING - lpIn;
        const float f1 = 1.0f - frac;
        for (int i = 0; i < Samples; ++i)
        {
            const float mixvalue = blepPTR[lpIn] * f1 + blepPTR[lpIn + 1] * frac;
            blepBuffer[(blepPos + i) & MASK] += mixvalue * scale;
            lpIn += B_OVERSAMPLING;
        }
        for (int i = Samples; i < N; ++i)
        {
            const float mixvalue = blepPTR[lpIn] * f1 + blepPTR[lpIn + 1] * frac;
            blepBuffer[(blepPos + i) & MASK] -= mixvalue * scale;
            lpIn += B_OVERSAMPLING;
        }
    }

    void applyBLAMP(float offset, float scale)
    {
        int lpIn = (int) (B_OVERSAMPLING * (offset));
        const float frac = offset * B_OVERSAMPLING - lpIn;
        const float f1 = 1.0f - frac;
        for (int i = 0; i < N; ++i)
        {
            const float mixvalue = blampPTR[lpIn] * f1 + blampPTR[lpIn + 1] * frac;
            blepBuffer[(blepPos + i) & MASK] += mixvalue * scale;
            lpIn += B_OVERSAMPLING;
        }
    }

    float getNextBlep()
    {
        blepBuffer[blepPos] = 0.0f;

        ++blepPos;
        blepPos &= MASK;

        return blepBuffer[blepPos];
    }

    DelayLine<float, Samples> delay;
    std::array<float, N> blepBuffer {};
    float const * blepPTR = blep;
    float const * blampPTR = blamp;
    int blepPos = 0;
    bool fall = false;
};
