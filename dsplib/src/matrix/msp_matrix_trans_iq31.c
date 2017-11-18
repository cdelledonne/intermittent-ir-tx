/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

#include <dsplib/inc/DSPLib.h>

/*
 * Perform matrix transposition of a source matrix.
 */
msp_status msp_matrix_trans_iq31(const msp_matrix_trans_iq31_params *params, const _iq31 *src, _iq31 *dst)
{
    uint16_t i;
    uint16_t rows;
    uint16_t cols;
    msp_status status;
    msp_deinterleave_iq31_params deinterParams;
    
    /* Extract matrix parameters. */
    rows = params->rows; 
    cols = params->cols;

    /* Initialize deinterleave parameters. */
    deinterParams.length = rows;
    deinterParams.numChannels = cols;
    
    /* Iterate through source columns and deinterleave to destination. */
    for (i = 0; i < cols; i++) {
        /* Transpose source columns to destination rows using deinterleave. */
        deinterParams.channel = i;
        status = msp_deinterleave_iq31(&deinterParams, src, &dst[i*rows]);
        if (status != MSP_SUCCESS) {
            return status;
        }
    }

    return MSP_SUCCESS;
}
