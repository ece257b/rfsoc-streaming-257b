/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "data_proc.h"

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

ap_uint<64> src_addr =  0x0;
ap_uint<64> dest_addr = 0XFFFFFFFFFFFF;
ap_uint<64> eth_type = 0xAEFE;
ap_uint<64> maxm = 180;

typedef ap_axis<64,0,0,0> axis_word;

void data_proc(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream) {
#pragma HLS INTERFACE mode=axis register_mode=both port=in_stream register
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis register_mode=both port=out_stream register
#pragma HLS INTERFACE ap_ctrl_none port=return

    static ap_uint<64> data_count = 0;
    axis_word output_word;

    if (data_count == 0) {
    	// s s s s s s d d
        output_word.data = src_addr << 16 | dest_addr >> 32;
    } else if (data_count == 1) {
    	// d d d d e e _ _
        output_word.data = dest_addr << 32 | eth_type << 16;

    } else {
        in_stream.read(output_word);
    }

    output_word.keep = 0xFF;  // All bytes are valid
    output_word.last = (data_count == maxm - 1) ? 1 : 0;  // Set TLAST when data_count reaches maximum possible value

    out_stream.write(output_word);

    data_count = (data_count + 1) % maxm;
}
