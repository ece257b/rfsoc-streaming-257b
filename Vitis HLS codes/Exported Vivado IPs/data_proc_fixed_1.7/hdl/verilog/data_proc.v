// ==============================================================
// RTL generated by Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2022.1 (64-bit)
// Version: 2022.1
// Copyright (C) Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// 
// ===========================================================

`timescale 1 ns / 1 ps 

(* CORE_GENERATION_INFO="data_proc_data_proc,hls_ip_2022_1,{HLS_INPUT_TYPE=cxx,HLS_INPUT_FLOAT=0,HLS_INPUT_FIXED=0,HLS_INPUT_PART=xczu28dr-ffvg1517-2-e,HLS_INPUT_CLOCK=6.000000,HLS_INPUT_ARCH=pipeline,HLS_SYN_CLOCK=1.909000,HLS_SYN_LAT=1,HLS_SYN_TPT=1,HLS_SYN_MEM=0,HLS_SYN_DSP=0,HLS_SYN_FF=7,HLS_SYN_LUT=104,HLS_VERSION=2022_1}" *)

module data_proc (
        ap_clk,
        ap_rst_n,
        in_stream_TDATA,
        in_stream_TVALID,
        in_stream_TREADY,
        in_stream_TKEEP,
        in_stream_TSTRB,
        in_stream_TLAST,
        out_stream_TDATA,
        out_stream_TVALID,
        out_stream_TREADY,
        out_stream_TKEEP,
        out_stream_TSTRB,
        out_stream_TLAST
);

parameter    ap_ST_iter0_fsm_state1 = 1'd1;
parameter    ap_ST_iter1_fsm_state2 = 2'd2;
parameter    ap_ST_iter1_fsm_state0 = 2'd1;

input   ap_clk;
input   ap_rst_n;
input  [63:0] in_stream_TDATA;
input   in_stream_TVALID;
output   in_stream_TREADY;
input  [7:0] in_stream_TKEEP;
input  [7:0] in_stream_TSTRB;
input  [0:0] in_stream_TLAST;
output  [63:0] out_stream_TDATA;
output   out_stream_TVALID;
input   out_stream_TREADY;
output  [7:0] out_stream_TKEEP;
output  [7:0] out_stream_TSTRB;
output  [0:0] out_stream_TLAST;

 reg    ap_rst_n_inv;
reg   [3:0] data_count_V;
reg    in_stream_TDATA_blk_n;
reg   [0:0] ap_CS_iter0_fsm;
wire    ap_CS_iter0_fsm_state1;
reg   [1:0] ap_CS_iter1_fsm;
wire    ap_CS_iter1_fsm_state2;
reg    out_stream_TDATA_blk_n;
reg    ap_predicate_op19_read_state1;
reg    ap_block_state1_pp0_stage0_iter0;
wire    regslice_both_out_stream_V_data_V_U_apdone_blk;
reg    ap_block_state2_pp0_stage0_iter1;
reg   [63:0] ap_phi_mux_output_word_data_V_phi_fu_98_p6;
wire   [63:0] ap_phi_reg_pp0_iter0_output_word_data_V_reg_94;
wire   [3:0] ret_V_fu_144_p3;
wire   [3:0] add_ln1560_fu_126_p2;
wire   [0:0] icmp_ln232_fu_132_p2;
wire   [3:0] add_ln232_fu_138_p2;
reg   [0:0] ap_NS_iter0_fsm;
reg   [1:0] ap_NS_iter1_fsm;
reg    ap_ST_iter0_fsm_state1_blk;
reg    ap_ST_iter1_fsm_state2_blk;
wire    regslice_both_in_stream_V_data_V_U_apdone_blk;
wire   [63:0] in_stream_TDATA_int_regslice;
wire    in_stream_TVALID_int_regslice;
reg    in_stream_TREADY_int_regslice;
wire    regslice_both_in_stream_V_data_V_U_ack_in;
wire    regslice_both_in_stream_V_keep_V_U_apdone_blk;
wire   [7:0] in_stream_TKEEP_int_regslice;
wire    regslice_both_in_stream_V_keep_V_U_vld_out;
wire    regslice_both_in_stream_V_keep_V_U_ack_in;
wire    regslice_both_in_stream_V_strb_V_U_apdone_blk;
wire   [7:0] in_stream_TSTRB_int_regslice;
wire    regslice_both_in_stream_V_strb_V_U_vld_out;
wire    regslice_both_in_stream_V_strb_V_U_ack_in;
wire    regslice_both_in_stream_V_last_V_U_apdone_blk;
wire   [0:0] in_stream_TLAST_int_regslice;
wire    regslice_both_in_stream_V_last_V_U_vld_out;
wire    regslice_both_in_stream_V_last_V_U_ack_in;
reg    out_stream_TVALID_int_regslice;
wire    out_stream_TREADY_int_regslice;
wire    regslice_both_out_stream_V_data_V_U_vld_out;
wire    regslice_both_out_stream_V_keep_V_U_apdone_blk;
wire    regslice_both_out_stream_V_keep_V_U_ack_in_dummy;
wire    regslice_both_out_stream_V_keep_V_U_vld_out;
wire    regslice_both_out_stream_V_strb_V_U_apdone_blk;
wire    regslice_both_out_stream_V_strb_V_U_ack_in_dummy;
wire    regslice_both_out_stream_V_strb_V_U_vld_out;
wire    regslice_both_out_stream_V_last_V_U_apdone_blk;
wire   [0:0] out_stream_TLAST_int_regslice;
wire    regslice_both_out_stream_V_last_V_U_ack_in_dummy;
wire    regslice_both_out_stream_V_last_V_U_vld_out;
wire    ap_ce_reg;

// power-on initialization
initial begin
#0 data_count_V = 4'd0;
#0 ap_CS_iter0_fsm = 1'd1;
#0 ap_CS_iter1_fsm = 2'd1;
end

data_proc_regslice_both #(
    .DataWidth( 64 ))
regslice_both_in_stream_V_data_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(in_stream_TDATA),
    .vld_in(in_stream_TVALID),
    .ack_in(regslice_both_in_stream_V_data_V_U_ack_in),
    .data_out(in_stream_TDATA_int_regslice),
    .vld_out(in_stream_TVALID_int_regslice),
    .ack_out(in_stream_TREADY_int_regslice),
    .apdone_blk(regslice_both_in_stream_V_data_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 8 ))
regslice_both_in_stream_V_keep_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(in_stream_TKEEP),
    .vld_in(in_stream_TVALID),
    .ack_in(regslice_both_in_stream_V_keep_V_U_ack_in),
    .data_out(in_stream_TKEEP_int_regslice),
    .vld_out(regslice_both_in_stream_V_keep_V_U_vld_out),
    .ack_out(in_stream_TREADY_int_regslice),
    .apdone_blk(regslice_both_in_stream_V_keep_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 8 ))
regslice_both_in_stream_V_strb_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(in_stream_TSTRB),
    .vld_in(in_stream_TVALID),
    .ack_in(regslice_both_in_stream_V_strb_V_U_ack_in),
    .data_out(in_stream_TSTRB_int_regslice),
    .vld_out(regslice_both_in_stream_V_strb_V_U_vld_out),
    .ack_out(in_stream_TREADY_int_regslice),
    .apdone_blk(regslice_both_in_stream_V_strb_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 1 ))
regslice_both_in_stream_V_last_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(in_stream_TLAST),
    .vld_in(in_stream_TVALID),
    .ack_in(regslice_both_in_stream_V_last_V_U_ack_in),
    .data_out(in_stream_TLAST_int_regslice),
    .vld_out(regslice_both_in_stream_V_last_V_U_vld_out),
    .ack_out(in_stream_TREADY_int_regslice),
    .apdone_blk(regslice_both_in_stream_V_last_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 64 ))
regslice_both_out_stream_V_data_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(ap_phi_mux_output_word_data_V_phi_fu_98_p6),
    .vld_in(out_stream_TVALID_int_regslice),
    .ack_in(out_stream_TREADY_int_regslice),
    .data_out(out_stream_TDATA),
    .vld_out(regslice_both_out_stream_V_data_V_U_vld_out),
    .ack_out(out_stream_TREADY),
    .apdone_blk(regslice_both_out_stream_V_data_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 8 ))
regslice_both_out_stream_V_keep_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(8'd255),
    .vld_in(out_stream_TVALID_int_regslice),
    .ack_in(regslice_both_out_stream_V_keep_V_U_ack_in_dummy),
    .data_out(out_stream_TKEEP),
    .vld_out(regslice_both_out_stream_V_keep_V_U_vld_out),
    .ack_out(out_stream_TREADY),
    .apdone_blk(regslice_both_out_stream_V_keep_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 8 ))
regslice_both_out_stream_V_strb_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(8'd255),
    .vld_in(out_stream_TVALID_int_regslice),
    .ack_in(regslice_both_out_stream_V_strb_V_U_ack_in_dummy),
    .data_out(out_stream_TSTRB),
    .vld_out(regslice_both_out_stream_V_strb_V_U_vld_out),
    .ack_out(out_stream_TREADY),
    .apdone_blk(regslice_both_out_stream_V_strb_V_U_apdone_blk)
);

data_proc_regslice_both #(
    .DataWidth( 1 ))
regslice_both_out_stream_V_last_V_U(
    .ap_clk(ap_clk),
    .ap_rst(ap_rst_n_inv),
    .data_in(out_stream_TLAST_int_regslice),
    .vld_in(out_stream_TVALID_int_regslice),
    .ack_in(regslice_both_out_stream_V_last_V_U_ack_in_dummy),
    .data_out(out_stream_TLAST),
    .vld_out(regslice_both_out_stream_V_last_V_U_vld_out),
    .ack_out(out_stream_TREADY),
    .apdone_blk(regslice_both_out_stream_V_last_V_U_apdone_blk)
);

always @ (posedge ap_clk) begin
    if (ap_rst_n_inv == 1'b1) begin
        ap_CS_iter0_fsm <= ap_ST_iter0_fsm_state1;
    end else begin
        ap_CS_iter0_fsm <= ap_NS_iter0_fsm;
    end
end

always @ (posedge ap_clk) begin
    if (ap_rst_n_inv == 1'b1) begin
        ap_CS_iter1_fsm <= ap_ST_iter1_fsm_state0;
    end else begin
        ap_CS_iter1_fsm <= ap_NS_iter1_fsm;
    end
end

always @ (posedge ap_clk) begin
    if ((~((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0)) | ((1'b1 == ap_CS_iter1_fsm_state2) & ((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0)))) & (1'b1 == ap_CS_iter0_fsm_state1))) begin
        data_count_V <= ret_V_fu_144_p3;
    end
end

always @ (*) begin
    if (((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0)))) begin
        ap_ST_iter0_fsm_state1_blk = 1'b1;
    end else begin
        ap_ST_iter0_fsm_state1_blk = 1'b0;
    end
end

always @ (*) begin
    if (((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0))) begin
        ap_ST_iter1_fsm_state2_blk = 1'b1;
    end else begin
        ap_ST_iter1_fsm_state2_blk = 1'b0;
    end
end

always @ (*) begin
    if ((data_count_V == 4'd0)) begin
        ap_phi_mux_output_word_data_V_phi_fu_98_p6 = 64'd18446462598732840960;
    end else if ((data_count_V == 4'd1)) begin
        ap_phi_mux_output_word_data_V_phi_fu_98_p6 = 64'd280027572731903;
    end else if ((~(data_count_V == 4'd1) & ~(data_count_V == 4'd0))) begin
        ap_phi_mux_output_word_data_V_phi_fu_98_p6 = in_stream_TDATA_int_regslice;
    end else begin
        ap_phi_mux_output_word_data_V_phi_fu_98_p6 = ap_phi_reg_pp0_iter0_output_word_data_V_reg_94;
    end
end

always @ (*) begin
    if ((~(data_count_V == 4'd1) & ~(data_count_V == 4'd0) & (1'b1 == ap_CS_iter0_fsm_state1))) begin
        in_stream_TDATA_blk_n = in_stream_TVALID_int_regslice;
    end else begin
        in_stream_TDATA_blk_n = 1'b1;
    end
end

always @ (*) begin
    if ((~((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0)) | ((1'b1 == ap_CS_iter1_fsm_state2) & ((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0)))) & (ap_predicate_op19_read_state1 == 1'b1) & (1'b1 == ap_CS_iter0_fsm_state1))) begin
        in_stream_TREADY_int_regslice = 1'b1;
    end else begin
        in_stream_TREADY_int_regslice = 1'b0;
    end
end

always @ (*) begin
    if (((1'b1 == ap_CS_iter1_fsm_state2) | (1'b1 == ap_CS_iter0_fsm_state1))) begin
        out_stream_TDATA_blk_n = out_stream_TREADY_int_regslice;
    end else begin
        out_stream_TDATA_blk_n = 1'b1;
    end
end

always @ (*) begin
    if ((~((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0)) | ((1'b1 == ap_CS_iter1_fsm_state2) & ((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0)))) & (1'b1 == ap_CS_iter0_fsm_state1))) begin
        out_stream_TVALID_int_regslice = 1'b1;
    end else begin
        out_stream_TVALID_int_regslice = 1'b0;
    end
end

always @ (*) begin
    case (ap_CS_iter0_fsm)
        ap_ST_iter0_fsm_state1 : begin
            ap_NS_iter0_fsm = ap_ST_iter0_fsm_state1;
        end
        default : begin
            ap_NS_iter0_fsm = 'bx;
        end
    endcase
end

always @ (*) begin
    case (ap_CS_iter1_fsm)
        ap_ST_iter1_fsm_state2 : begin
            if ((~((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0))) & ~((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0)) & (1'b1 == ap_CS_iter0_fsm_state1))) begin
                ap_NS_iter1_fsm = ap_ST_iter1_fsm_state2;
            end else if ((~((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0)) & ((1'b0 == ap_CS_iter0_fsm_state1) | ((1'b1 == ap_CS_iter0_fsm_state1) & ((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0))))))) begin
                ap_NS_iter1_fsm = ap_ST_iter1_fsm_state0;
            end else begin
                ap_NS_iter1_fsm = ap_ST_iter1_fsm_state2;
            end
        end
        ap_ST_iter1_fsm_state0 : begin
            if ((~((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0)) | ((1'b1 == ap_CS_iter1_fsm_state2) & ((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0)))) & (1'b1 == ap_CS_iter0_fsm_state1))) begin
                ap_NS_iter1_fsm = ap_ST_iter1_fsm_state2;
            end else begin
                ap_NS_iter1_fsm = ap_ST_iter1_fsm_state0;
            end
        end
        default : begin
            ap_NS_iter1_fsm = 'bx;
        end
    endcase
end

assign add_ln1560_fu_126_p2 = (data_count_V + 4'd1);

assign add_ln232_fu_138_p2 = (data_count_V + 4'd7);

assign ap_CS_iter0_fsm_state1 = ap_CS_iter0_fsm[32'd0];

assign ap_CS_iter1_fsm_state2 = ap_CS_iter1_fsm[32'd1];

always @ (*) begin
    ap_block_state1_pp0_stage0_iter0 = ((out_stream_TREADY_int_regslice == 1'b0) | ((ap_predicate_op19_read_state1 == 1'b1) & (in_stream_TVALID_int_regslice == 1'b0)));
end

always @ (*) begin
    ap_block_state2_pp0_stage0_iter1 = ((regslice_both_out_stream_V_data_V_U_apdone_blk == 1'b1) | (out_stream_TREADY_int_regslice == 1'b0));
end

assign ap_phi_reg_pp0_iter0_output_word_data_V_reg_94 = 'bx;

always @ (*) begin
    ap_predicate_op19_read_state1 = (~(data_count_V == 4'd1) & ~(data_count_V == 4'd0));
end

always @ (*) begin
    ap_rst_n_inv = ~ap_rst_n;
end

assign icmp_ln232_fu_132_p2 = ((add_ln1560_fu_126_p2 < 4'd10) ? 1'b1 : 1'b0);

assign in_stream_TREADY = regslice_both_in_stream_V_data_V_U_ack_in;

assign out_stream_TLAST_int_regslice = ((data_count_V == 4'd9) ? 1'b1 : 1'b0);

assign out_stream_TVALID = regslice_both_out_stream_V_data_V_U_vld_out;

assign ret_V_fu_144_p3 = ((icmp_ln232_fu_132_p2[0:0] == 1'b1) ? add_ln1560_fu_126_p2 : add_ln232_fu_138_p2);

endmodule //data_proc
