#pragma once

// == 1 / log2(10)
#define ONE_OVER_LOG2_10 0.3010299956639812f

// compute log2(x) by reducing x to [0.75, 1.5), then divide by log2(10)
// See: https://tech.ebayinc.com/engineering/fast-approximate-logarithms-part-iii-the-formulas/
// This version uses only multiplies
float log10f_fast(float x)
{
    constexpr float a = 0.338531;
    constexpr float b = -0.741619;
    constexpr float c = 1.445866;
#define FNM fexp + (((a * signif) + b) * signif + c) * signif

    float signif, fexp;
    int exp;
    float lg2;
    union {
        float f;
        unsigned int i;
    } ux1, ux2;
    int greater; // really a boolean
    /* 
     * Assume IEEE representation, which is sgn(1):exp(8):frac(23)
     * representing (1+frac)*2^(exp-127).  Call 1+frac the significand
     */

    // get exponent
    ux1.f = x;
    exp = (ux1.i & 0x7F800000) >> 23;
    // actual exponent is exp-127, will subtract 127 later

    greater = ux1.i & 0x00400000; // true if signif > 1.5
    if (greater)
    {
        // signif >= 1.5 so need to divide by 2.  Accomplish this by
        // stuffing exp = 126 which corresponds to an exponent of -1
        ux2.i = (ux1.i & 0x007FFFFF) | 0x3f000000;
        signif = ux2.f;
        fexp = exp - 126; // 126 instead of 127 compensates for division by 2
        signif = signif - 1.0;
        lg2 = FNM;
    }
    else
    {
        // get signif by stuffing exp = 127 which corresponds to an exponent of 0
        ux2.i = (ux1.i & 0x007FFFFF) | 0x3f800000;
        signif = ux2.f;
        fexp = exp - 127;
        signif = signif - 1.0;
        lg2 = FNM;
    }
    // last two lines of each branch are common code, but optimize better
    //  when duplicated, at least when using gcc
    return lg2 * ONE_OVER_LOG2_10;
}
