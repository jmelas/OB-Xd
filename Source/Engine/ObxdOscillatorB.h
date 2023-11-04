/*
    ==============================================================================
    This file is part of Obxd synthesizer.

    Copyright � 2013-2014 Filatov Vadim

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

#include "ObxdVoice.h"
#include "SynthEngine.h"
#include "AudioUtils.h"
#include "BlepData.h"
#include "DelayLine.h"
#include "SawOsc.h"
#include "PulseOsc.h"
#include "TriangleOsc.h"

class ObxdOscillatorB
{
private:
    float SampleRate;
    float sampleRateInv;

    float x1, x2;

    float osc1Factor;
    float osc2Factor;

    float pw1w = 0, pw2w = 0;

    DelayLine<float, Samples> xmodd;
    DelayLine<bool, Samples> syncd;
    DelayLine<float, Samples> syncFracd;
    DelayLine<float, Samples> cvd;

    Random wn;
    SawOsc o1s, o2s;
    PulseOsc o1p, o2p;
    TriangleOsc o1t, o2t;

    float dirt = 0.1;

public:
    float osc2Det = 0;
    bool osc1Saw = false, osc2Saw = false, osc1Pul = false, osc2Pul = false;
    float osc1p = 10, osc2p = 10;

    float pto1 = 0, pto2 = 0;
    float pw1 = 0, pw2 = 0;

    float pulseWidth = 0;
    bool hardSync = false;
    float xmod = 0;

    float notePlaying = 30;
    bool quantizeCw = false;

    float o1mx = 0, o2mx = 0, nmx = 0;

    float tune = 0;//+-1
    int oct = 0;
    float totalDetune = 0;

    ObxdOscillatorB()
    {
        wn = Random(Random::getSystemRandom().nextInt64());
        osc1Factor = wn.nextFloat() - 0.5;
        osc2Factor = wn.nextFloat() - 0.5;
        x1 = wn.nextFloat();
        x2 = wn.nextFloat();
    }

    void setDecimation()
    {
        o1p.setDecimation();
        o1t.setDecimation();
        o1s.setDecimation();
        o2p.setDecimation();
        o2t.setDecimation();
        o2s.setDecimation();
    }

    void removeDecimation()
    {
        o1p.removeDecimation();
        o1t.removeDecimation();
        o1s.removeDecimation();
        o2p.removeDecimation();
        o2t.removeDecimation();
        o2s.removeDecimation();
    }

    void setSampleRate(float sr)
    {
        SampleRate = sr;
        sampleRateInv = 1.0f / SampleRate;
    }

    float ProcessSample()
    {
        float noiseGen = wn.nextFloat() - 0.5;
        float pitch1 = getPitch(dirt * noiseGen + notePlaying + (quantizeCw ? ((int) (osc1p)) : osc1p) + pto1 + tune + oct + totalDetune * osc1Factor);
        bool hsr = false;
        float hsfrac = 0;
        float fs = jmin(pitch1 * (sampleRateInv), 0.45f);
        x1 += fs;
        hsfrac = 0;
        float osc1mix = 0.0f;
        float pwcalc = jlimit<float>(0.1f, 1.0f, (pulseWidth + pw1) * 0.5f + 0.5f);

        if (osc1Pul)
            o1p.processMaster(x1, fs, pwcalc, pw1w);
        if (osc1Saw)
            o1s.processMaster(x1, fs);
        else if (!osc1Pul)
            o1t.processMaster(x1, fs);

        if (x1 >= 1.0f)
        {
            x1 -= 1.0f;
            hsfrac = x1 / fs;
            hsr = true;
        }

        pw1w = pwcalc;

        hsr &= hardSync;
        //Delaying our hard sync gate signal and frac
        hsr = syncd.tick(hsr) != 0.0f;
        hsfrac = syncFracd.tick(hsfrac);

        if (osc1Pul)
            osc1mix += o1p.tick(x1, pwcalc);
        if (osc1Saw)
            osc1mix += o1s.tick(x1);
        else if (!osc1Pul)
            osc1mix = o1t.tick(x1);
        //Pitch control needs additional delay buffer to compensate
        //This will give us less aliasing on xmod
        //Hard sync gate signal delayed too
        noiseGen = wn.nextFloat() - 0.5;
        float pitch2 = getPitch(cvd.tick(dirt * noiseGen + notePlaying + osc2Det + (quantizeCw ? ((int) (osc2p)) : osc2p) + pto2 + osc1mix * xmod + tune + oct + totalDetune * osc2Factor));

        fs = jmin(pitch2 * (sampleRateInv), 0.45f);

        pwcalc = jlimit<float>(0.1f, 1.0f, (pulseWidth + pw2) * 0.5f + 0.5f);

        float osc2mix = 0.0f;

        x2 += fs;

        if (osc2Pul)
            o2p.processSlave(x2, fs, hsr, hsfrac, pwcalc, pw2w);
        if (osc2Saw)
            o2s.processSlave(x2, fs, hsr, hsfrac);
        else if (!osc2Pul)
            o2t.processSlave(x2, fs, hsr, hsfrac);

        if (x2 >= 1.0f)
            x2 -= 1.0;

        pw2w = pwcalc;
        //On hard sync reset slave phase is affected that way
        if (hsr)
        {
            float fracMaster = (fs * hsfrac);
            x2 = fracMaster;
        }
        //Delaying osc1 signal
        //And getting delayed back
        osc1mix = xmodd.tick(osc1mix);

        if (osc2Pul)
            osc2mix += o2p.tick(x2, pwcalc);
        if (osc2Saw)
            osc2mix += o2s.tick(x2);
        else if (!osc2Pul)
            osc2mix = o2t.tick(x2);

        //mixing
        float res = o1mx * osc1mix + o2mx * osc2mix + noiseGen * (nmx * 1.3 + 0.0006);
        return res * 3;
    }
};
