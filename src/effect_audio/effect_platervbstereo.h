/*  Stereo plate reverb for Teensy 4
 *
 *  Adapted for use in MiniDexed (Holger Wirtz <wirtz@parasitstudio.de>)
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2020 by Piotr Zapart
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/***
 * Algorithm based on plate reverbs developed for SpinSemi FV-1 DSP chip
 * 
 * Allpass + modulated delay line based lush plate reverb
 * 
 * Input parameters are float in range 0.0 to 1.0:
 * 
 * size - reverb time
 * hidamp - hi frequency loss in the reverb tail
 * lodamp - low frequency loss in the reverb tail
 * lowpass - output/master lowpass filter, useful for darkening the reverb sound 
 * diffusion - lower settings will make the reverb tail more "echoey", optimal value 0.65
 * 
 */

#ifndef _EFFECT_PLATERVBSTEREO_H
#define _EFFECT_PLATERVBSTEREO_H

#include <stdint.h>
#include <arm_math.h>
#include "../common.h"
#include "effect_base.h"

/***
 * Loop delay modulation: comment/uncomment to switch sin/cos 
 * modulation for the 1st or 2nd tap, 3rd tap is always modulated
 * more modulation means more chorus type sounding reverb tail
 */
//#define TAP1_MODULATED
#define TAP2_MODULATED

class AudioEffectPlateReverb : public AudioEffect
{
public:
    // ID must be unique for each AudioEffect
    static const unsigned ID = 7;
    static constexpr const char* NAME = "Reverb";

    enum Param
    {
        BYPASS,
        SIZE,
        HIGH_DAMP,
        LOW_DAMP,
        LOW_PASS,
        DIFFUSION,
        MIX,
        LEVEL,
        UNKNOWN
    };

    AudioEffectPlateReverb(float32_t samplerate);
    //virtual ~AudioEffectPlateReverb();

    virtual unsigned getId()
    {
        return AudioEffectPlateReverb::ID;
    }

    virtual std::string getName()
    {
        return AudioEffectPlateReverb::NAME;
    }

    virtual void initializeSendFX();

    virtual void setParameter(unsigned param, unsigned value);
    virtual unsigned getParameter(unsigned param);

    void doReverb(const float32_t* inblockL, const float32_t* inblockR, float32_t* rvbblockL, float32_t* rvbblockR,uint16_t len);

    void size(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = mapfloat(n, 0.0f, 1.0f, 0.2f, rv_time_k_max);
        float32_t attn = mapfloat(n, 0.0f, rv_time_k_max, 0.5f, 0.25f);
        rv_time_k = n;
        input_attn = attn;
    }

    void hidamp(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        lp_hidamp_k = 1.0f - n;
    }
    
    void lodamp(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        lp_lodamp_k = -n;
        rv_time_scaler = 1.0f - n * 0.12f;        // limit the max reverb time, otherwise it will clip
    }

    void lowpass(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = mapfloat(n*n*n, 0.0f, 1.0f, 0.05f, 1.0f);
        master_lowpass_f = n;
    }
    
    void diffusion(float n)
    {
        n = constrain(n, 0.0f, 1.0f);
        n = mapfloat(n, 0.0f, 1.0f, 0.005f, 0.65f);
        in_allp_k = n;
        loop_allp_k = n;
    }

    void level(float n)
    {
        reverb_level = constrain(n, 0.0f, 1.0f);
    }

    float32_t get_size(void) {return rv_time_k;}
    float32_t get_level(void) {return reverb_level;}

protected:
    virtual size_t getParametersSize()
    {
        return AudioEffectPlateReverb::Param::UNKNOWN;
    }
    virtual void doProcess(const float32_t* inblockL, const float32_t* inblockR, float32_t* outblockL, float32_t* outblockR, uint16_t len);

private:
    float32_t reverb_level;

    unsigned sizeValue;
    unsigned hidampValue;
    unsigned lodampValue;
    unsigned lowpassValue;
    unsigned diffusionValue;
    float32_t mix;
    float32_t dryMix;
    float32_t wetMix;
    
    void setMix(float32_t mix)
    {
        this->mix = mix;
        if (this->mix <= 0.5f)
        {
            this->dryMix = 1.0f;
            this->wetMix = this->mix * 2;
        }
        else
        {
            this->dryMix = 1.0f - ((this->mix - 0.5f) * 2);
            this->wetMix = 1.0f;
        }
    }

    float32_t input_attn;

    float32_t in_allp_k;            // input allpass coeff 
    float32_t in_allp1_bufL[224];   // input allpass buffers
    float32_t in_allp2_bufL[420];
    float32_t in_allp3_bufL[856];
    float32_t in_allp4_bufL[1089];
    uint16_t in_allp1_idxL;
    uint16_t in_allp2_idxL;
    uint16_t in_allp3_idxL;
    uint16_t in_allp4_idxL;
    float32_t in_allp_out_L;    // L allpass chain output
    float32_t in_allp1_bufR[156]; // input allpass buffers
    float32_t in_allp2_bufR[520];
    float32_t in_allp3_bufR[956];
    float32_t in_allp4_bufR[1289];
    uint16_t in_allp1_idxR;
    uint16_t in_allp2_idxR;
    uint16_t in_allp3_idxR;
    uint16_t in_allp4_idxR;
    float32_t in_allp_out_R;    // R allpass chain output
    float32_t lp_allp1_buf[2303]; // loop allpass buffers
    float32_t lp_allp2_buf[2905];
    float32_t lp_allp3_buf[3175];
    float32_t lp_allp4_buf[2398];
    uint16_t lp_allp1_idx;
    uint16_t lp_allp2_idx;
    uint16_t lp_allp3_idx;
    uint16_t lp_allp4_idx;
    float32_t loop_allp_k;         // loop allpass coeff
    float32_t lp_allp_out;
    float32_t lp_dly1_buf[3423];
    float32_t lp_dly2_buf[4589];
    float32_t lp_dly3_buf[4365];
    float32_t lp_dly4_buf[3698];
    uint16_t lp_dly1_idx;
    uint16_t lp_dly2_idx;
    uint16_t lp_dly3_idx;
    uint16_t lp_dly4_idx;

    const uint16_t lp_dly1_offset_L = 201;      // delay line tap offets
    const uint16_t lp_dly2_offset_L = 145;
    const uint16_t lp_dly3_offset_L = 1897;
    const uint16_t lp_dly4_offset_L = 280;

    const uint16_t lp_dly1_offset_R = 1897;
    const uint16_t lp_dly2_offset_R = 1245;
    const uint16_t lp_dly3_offset_R = 487;
    const uint16_t lp_dly4_offset_R = 780;  

    float32_t lp_hidamp_k;       // loop high band damping coeff
    float32_t lp_lodamp_k;       // loop low baand damping coeff

    float32_t lpf1;             // lowpass filters
    float32_t lpf2;
    float32_t lpf3;
    float32_t lpf4;

    float32_t hpf1;             // highpass filters
    float32_t hpf2;
    float32_t hpf3;
    float32_t hpf4;

    float32_t lp_lowpass_f;      // loop lowpass scaled frequency
    float32_t lp_hipass_f;       // loop highpass scaled frequency 

    float32_t master_lowpass_f;
    float32_t master_lowpass_l;
    float32_t master_lowpass_r;

    const float32_t rv_time_k_max = 0.95f;
    float32_t rv_time_k;         // reverb time coeff
    float32_t rv_time_scaler;    // with high lodamp settings lower the max reverb time to avoid clipping

    uint32_t lfo1_phase_acc;     // LFO 1
    uint32_t lfo1_adder;

    uint32_t lfo2_phase_acc;    // LFO 2
    uint32_t lfo2_adder;
};

#endif // _EFFECT_PLATEREV_H
