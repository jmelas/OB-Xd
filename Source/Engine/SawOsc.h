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

class SawOsc
{
    static constexpr int N = Samples * 2;
    static constexpr int MASK = N - 1;

public:
    void setDecimation() { blepPTR = blepd2; }
    void removeDecimation() { blepPTR = blep; }

    float tick(float x)
    {
        const float v = x - 0.5;
        return delay.tick(v) - getNextBlep();
    }

    void processMaster(float x, float delta)
    {
        if (x >= 1.0f)
        {
            x -= 1.0f;
            applyBLEP(x / delta, 1);
        }
    }

    void processSlave(float x, float delta, bool hardSyncReset, float hardSyncFrac)
    {
        if (x >= 1.0f)
        {
            x -= 1.0f;

            if (! hardSyncReset || x / delta > hardSyncFrac) //de morgan processed equation
            {
                applyBLEP(x / delta, 1);
            }
            else
            {
                //if transition did not occur
                x += 1;
            }
        }

        if (hardSyncReset)
        {
            const float fracMaster = delta * hardSyncFrac;
            const float trans = x - fracMaster;
            applyBLEP(hardSyncFrac, trans);
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
    int blepPos = 0;
};
