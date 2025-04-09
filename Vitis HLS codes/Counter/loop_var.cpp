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

#include "loop_var.h"
/*
//din_t A[N], dsel_t width
ap_int<8> loop_var() {

  //dout_t out_accum=0;
  //dsel_t x;
  static ap_int<8> count=0;
  count++;

  LOOP_X:for (x=0;x<width; x++) {
      out_accum += A[x];
  }

  return count;
}
*/
/*
#include <hls_stream.h>
#include <ap_int.h>

void loop_var(hls::stream<ap_uint<8>> &out_stream) {
#pragma HLS INTERFACE mode=axis register_mode=both port=out_stream register
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
    //#pragma HLS INTERFACE axis port=out_stream
    //#pragma HLS INTERFACE s_axilite port=reset
    //#pragma HLS INTERFACE s_axilite port=return
    //#pragma HLS PIPELINE II=1

    static ap_uint<8> count = 0;

    count++;

    out_stream.write(count);  // Send count over AXI Stream
}
*/

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

typedef ap_axis<64,0,0,0> axis_word;

//struct axis_word {
//    ap_uint<64> data;
//    ap_uint<8>  keep;
//    ap_uint<1>  last;
//};

void counter(hls::stream<axis_word> &out_stream) {
#pragma HLS PIPELINE II=1 style=flp
#pragma HLS INTERFACE mode=axis register_mode=both port=out_stream register
#pragma HLS INTERFACE ap_ctrl_none port=return

    static ap_uint<64> count = 0;
    axis_word output_word;
    if(count<256){
    	count++;
    }
    else{
    	// Reset counter
    	count = 0;
    }

    output_word.data = count;
    output_word.keep = 0xFF;  // All bytes are valid
    output_word.last = (count == 0xFF) ? 1 : 0;  // Set TLAST when count reaches maximum value

    out_stream.write(output_word);
}
