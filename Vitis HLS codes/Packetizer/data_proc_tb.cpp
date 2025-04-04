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
#include <ap_axi_sdata.h>
#include <hls_stream.h>

typedef ap_axis<64,0,0,0> axis_word;

int main () {
  hls::stream<axis_word> input;
  int N = 512;

  for(int i=0; i<N;++i) {
    axis_word w;
    w.data = i;
    input.write(w);
  }
  // Save the results to a file
  ofstream FILE;
  FILE.open ("result.dat");

  // Call the function
  hls::stream<axis_word> output;
  for(int i=0; i<N;++i) {
    axis_word o;
    data_proc(input, output);
    output.read(o);

    FILE << static_cast<int>(o.data) << " " << static_cast<int>(o.last) << endl;
  }
  FILE.close();

  // Compare the results file with the golden results
  int retval = system("diff --brief -w result.dat result.golden.dat");
  if (retval != 0) {
    cout << "Test failed  !!!" << endl;
    retval=1;
  } else {
    cout << "Test passed !" << endl;
  }

  // Return 0 if the test passed
  return retval;
}

