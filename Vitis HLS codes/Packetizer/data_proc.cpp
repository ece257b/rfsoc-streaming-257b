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
// Actual eth type = 0xAEFE, flipped it here for convenience in sending
ap_uint<64> eth_type = 0xFEAE;
ap_uint<64> maxm = 10;

typedef ap_axis<64,0,0,0> axis_word;

void data_proc(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream) {
#pragma HLS PIPELINE II=1 style=flp
#pragma HLS INTERFACE mode=axis register_mode=both port=in_stream register
#pragma HLS INTERFACE mode=ap_ctrl_none port=return
#pragma HLS INTERFACE mode=axis register_mode=both port=out_stream register
#pragma HLS INTERFACE ap_ctrl_none port=return

    static ap_uint<64> data_count = 0;
    axis_word output_word;

    if (data_count == 0) {
    	// Send opposite of what you want to receive (Populate from right side)
    	// Want to receive [s s s s s s | d d] [d d d d | e e | 0 0]
    	// Transmit this   [d d | s s s s s s] [0 0 | e e | d d d d]
    	// d d s s s s s s
    	// ff ff 00 00 00 00 00 00
        output_word.data = src_addr | ((dest_addr >> 32) << 48);
    } else if (data_count == 1) {
    	// - - e e d d d d
    	// 00 00 fe ae ff ff ff ff
        output_word.data = (dest_addr & 0x0000FFFFFFFF) | eth_type << 32;

    } else {
        in_stream.read(output_word);
    }

    output_word.keep = 0xFF;  // All bytes are valid
    output_word.strb = 0xFF;
    output_word.last = (data_count == maxm - 1) ? 1 : 0;  // Set TLAST when data_count reaches maximum possible value

    out_stream.write(output_word);

    data_count = (data_count + 1) % maxm;
}


//#include "data_proc.h"
//
//#include <hls_stream.h>
//#include <ap_axi_sdata.h>
//#include <ap_int.h>
//
//ap_uint<64> src_addr =  0x0;
//ap_uint<64> dest_addr = 0XFFFFFFFFFFFF;
////ap_uint<64> eth_type = 0xAEFE;
//ap_uint<64> eth_type = 0xFEAE;
////const ap_uint<64> maxm = 180;
////const int maxm = 180;
//const int maxm = 10;
//
////const ap_uint<8> buff_len = 100;
//
//typedef ap_axis<64,0,0,0> axis_word;
//
//void data_proc(hls::stream<axis_word> &in_stream, hls::stream<axis_word> &out_stream) {
//#pragma HLS INTERFACE mode=axis register_mode=both port=in_stream register
//#pragma HLS INTERFACE mode=ap_ctrl_none port=return
//#pragma HLS INTERFACE mode=axis register_mode=both port=out_stream register
//#pragma HLS INTERFACE ap_ctrl_none port=return
//
//
//	static ap_uint<64> buffer[maxm];
//    static ap_uint<64> data_count = 0;
//    axis_word input_word;
//    axis_word output_word;
//
//
//	// read incoming data and save to buffer
//	in_stream.read(input_word);
//	buffer[data_count] = input_word.data;
//
//	// Once all the data is read, write it all out to the outstream
//    if (data_count == maxm - 1) {
//
//		output_word.keep = 0xFF;  // All bytes are valid
//		output_word.last = 0;  // Set TLAST when data_count reaches maximum possible value
//
//		// Header part 1
//    	output_word.data = dest_addr << 16 | src_addr >> 32;
//    	out_stream.write(output_word);
//
//    	// Header part 2
//    	output_word.data = src_addr << 32 | eth_type << 16;
//    	out_stream.write(output_word);
//
//    	// Writing buffer data to outstream
//    	for (int i = 0; i < maxm; i++) {
//    		output_word.data = buffer[i];
//    		if(i == maxm - 1) {
//    			output_word.last = 1;
//    		}
//    		out_stream.write(output_word);
//    	}
//    }
//	data_count = (data_count + 1) % maxm;
//}
