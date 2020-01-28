/// @file
/// @brief Declare assembly language interrupt handlers.
///
/// There isn't much benefit to describing these in detail, so they are not documented further.
#pragma once

#include <stdint.h>

/// @cond

// Specialised interrupt handlers:
extern "C" void asm_proc_page_fault_handler();
extern "C" void proc_page_fault_handler(uint64_t fault_code, uint64_t fault_addr, uint64_t fault_instruction);
extern "C" void asm_task_switch_interrupt();

// IRQ handlers
extern "C" void asm_proc_handle_irq_0();
extern "C" void asm_proc_handle_irq_1();
extern "C" void asm_proc_handle_irq_2();
extern "C" void asm_proc_handle_irq_3();
extern "C" void asm_proc_handle_irq_4();
extern "C" void asm_proc_handle_irq_5();
extern "C" void asm_proc_handle_irq_6();
extern "C" void asm_proc_handle_irq_7();
extern "C" void asm_proc_handle_irq_8();
extern "C" void asm_proc_handle_irq_9();
extern "C" void asm_proc_handle_irq_10();
extern "C" void asm_proc_handle_irq_11();
extern "C" void asm_proc_handle_irq_12();
extern "C" void asm_proc_handle_irq_13();
extern "C" void asm_proc_handle_irq_14();
extern "C" void asm_proc_handle_irq_15();

// Exception handlers:
extern "C" void asm_proc_div_by_zero_fault_handler();  // 0
extern "C" void proc_div_by_zero_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_debug_fault_handler(); // 1
extern "C" void proc_debug_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_nmi_int_handler(); // 2
extern "C" void asm_proc_brkpt_trap_handler(); // 3
extern "C" void proc_brkpt_trap_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_overflow_trap_handler(); // 4
extern "C" void proc_overflow_trap_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_bound_range_fault_handler(); // 5
extern "C" void proc_bound_range_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_invalid_opcode_fault_handler(); // 6
extern "C" void proc_invalid_opcode_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_device_not_avail_fault_handler(); // 7
extern "C" void proc_device_not_avail_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_double_fault_abort_handler(); // 8
extern "C" void proc_double_fault_abort_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);
extern "C" void asm_proc_invalid_tss_fault_handler(); // 10
extern "C" void proc_invalid_tss_fault_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);
extern "C" void asm_proc_seg_not_present_fault_handler(); // 11
extern "C" void proc_seg_not_present_fault_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);
extern "C" void asm_proc_ss_fault_handler(); // 12
extern "C" void proc_ss_fault_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);
extern "C" void asm_proc_gen_prot_fault_handler(); // 13
extern "C" void proc_gen_prot_fault_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);
extern "C" void asm_proc_fp_except_fault_handler(); // 16
extern "C" void proc_fp_except_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_align_check_fault_handler(); // 17
extern "C" void proc_align_check_fault_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);
extern "C" void asm_proc_machine_check_abort_handler(); // 18
extern "C" void proc_machine_check_abort_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_simd_fpe_fault_handler(); // 19
extern "C" void proc_simd_fpe_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_virt_except_fault_handler(); // 20
extern "C" void proc_virt_except_fault_handler(uint64_t rip, uint64_t rsp, uint64_t opt, uint64_t k_rsp);
extern "C" void asm_proc_security_fault_handler(); // 30
extern "C" void proc_security_fault_handler(uint64_t rip, uint64_t rsp, uint64_t err_code, uint64_t k_rsp);

// Boring plain interrupt handlers.
extern "C" void asm_proc_interrupt_0_handler();
extern "C" void asm_proc_interrupt_1_handler();
extern "C" void asm_proc_interrupt_2_handler();
extern "C" void asm_proc_interrupt_3_handler();
extern "C" void asm_proc_interrupt_4_handler();
extern "C" void asm_proc_interrupt_5_handler();
extern "C" void asm_proc_interrupt_6_handler();
extern "C" void asm_proc_interrupt_7_handler();
extern "C" void asm_proc_interrupt_8_handler();
extern "C" void asm_proc_interrupt_9_handler();
extern "C" void asm_proc_interrupt_10_handler();
extern "C" void asm_proc_interrupt_11_handler();
extern "C" void asm_proc_interrupt_12_handler();
extern "C" void asm_proc_interrupt_13_handler();
extern "C" void asm_proc_interrupt_14_handler();
extern "C" void asm_proc_interrupt_15_handler();
extern "C" void asm_proc_interrupt_16_handler();
extern "C" void asm_proc_interrupt_17_handler();
extern "C" void asm_proc_interrupt_18_handler();
extern "C" void asm_proc_interrupt_19_handler();
extern "C" void asm_proc_interrupt_20_handler();
extern "C" void asm_proc_interrupt_21_handler();
extern "C" void asm_proc_interrupt_22_handler();
extern "C" void asm_proc_interrupt_23_handler();
extern "C" void asm_proc_interrupt_24_handler();
extern "C" void asm_proc_interrupt_25_handler();
extern "C" void asm_proc_interrupt_26_handler();
extern "C" void asm_proc_interrupt_27_handler();
extern "C" void asm_proc_interrupt_28_handler();
extern "C" void asm_proc_interrupt_29_handler();
extern "C" void asm_proc_interrupt_30_handler();
extern "C" void asm_proc_interrupt_31_handler();
extern "C" void asm_proc_interrupt_32_handler();
extern "C" void asm_proc_interrupt_33_handler();
extern "C" void asm_proc_interrupt_34_handler();
extern "C" void asm_proc_interrupt_35_handler();
extern "C" void asm_proc_interrupt_36_handler();
extern "C" void asm_proc_interrupt_37_handler();
extern "C" void asm_proc_interrupt_38_handler();
extern "C" void asm_proc_interrupt_39_handler();
extern "C" void asm_proc_interrupt_40_handler();
extern "C" void asm_proc_interrupt_41_handler();
extern "C" void asm_proc_interrupt_42_handler();
extern "C" void asm_proc_interrupt_43_handler();
extern "C" void asm_proc_interrupt_44_handler();
extern "C" void asm_proc_interrupt_45_handler();
extern "C" void asm_proc_interrupt_46_handler();
extern "C" void asm_proc_interrupt_47_handler();
extern "C" void asm_proc_interrupt_48_handler();
extern "C" void asm_proc_interrupt_49_handler();
extern "C" void asm_proc_interrupt_50_handler();
extern "C" void asm_proc_interrupt_51_handler();
extern "C" void asm_proc_interrupt_52_handler();
extern "C" void asm_proc_interrupt_53_handler();
extern "C" void asm_proc_interrupt_54_handler();
extern "C" void asm_proc_interrupt_55_handler();
extern "C" void asm_proc_interrupt_56_handler();
extern "C" void asm_proc_interrupt_57_handler();
extern "C" void asm_proc_interrupt_58_handler();
extern "C" void asm_proc_interrupt_59_handler();
extern "C" void asm_proc_interrupt_60_handler();
extern "C" void asm_proc_interrupt_61_handler();
extern "C" void asm_proc_interrupt_62_handler();
extern "C" void asm_proc_interrupt_63_handler();
extern "C" void asm_proc_interrupt_64_handler();
extern "C" void asm_proc_interrupt_65_handler();
extern "C" void asm_proc_interrupt_66_handler();
extern "C" void asm_proc_interrupt_67_handler();
extern "C" void asm_proc_interrupt_68_handler();
extern "C" void asm_proc_interrupt_69_handler();
extern "C" void asm_proc_interrupt_70_handler();
extern "C" void asm_proc_interrupt_71_handler();
extern "C" void asm_proc_interrupt_72_handler();
extern "C" void asm_proc_interrupt_73_handler();
extern "C" void asm_proc_interrupt_74_handler();
extern "C" void asm_proc_interrupt_75_handler();
extern "C" void asm_proc_interrupt_76_handler();
extern "C" void asm_proc_interrupt_77_handler();
extern "C" void asm_proc_interrupt_78_handler();
extern "C" void asm_proc_interrupt_79_handler();
extern "C" void asm_proc_interrupt_80_handler();
extern "C" void asm_proc_interrupt_81_handler();
extern "C" void asm_proc_interrupt_82_handler();
extern "C" void asm_proc_interrupt_83_handler();
extern "C" void asm_proc_interrupt_84_handler();
extern "C" void asm_proc_interrupt_85_handler();
extern "C" void asm_proc_interrupt_86_handler();
extern "C" void asm_proc_interrupt_87_handler();
extern "C" void asm_proc_interrupt_88_handler();
extern "C" void asm_proc_interrupt_89_handler();
extern "C" void asm_proc_interrupt_90_handler();
extern "C" void asm_proc_interrupt_91_handler();
extern "C" void asm_proc_interrupt_92_handler();
extern "C" void asm_proc_interrupt_93_handler();
extern "C" void asm_proc_interrupt_94_handler();
extern "C" void asm_proc_interrupt_95_handler();
extern "C" void asm_proc_interrupt_96_handler();
extern "C" void asm_proc_interrupt_97_handler();
extern "C" void asm_proc_interrupt_98_handler();
extern "C" void asm_proc_interrupt_99_handler();
extern "C" void asm_proc_interrupt_100_handler();
extern "C" void asm_proc_interrupt_101_handler();
extern "C" void asm_proc_interrupt_102_handler();
extern "C" void asm_proc_interrupt_103_handler();
extern "C" void asm_proc_interrupt_104_handler();
extern "C" void asm_proc_interrupt_105_handler();
extern "C" void asm_proc_interrupt_106_handler();
extern "C" void asm_proc_interrupt_107_handler();
extern "C" void asm_proc_interrupt_108_handler();
extern "C" void asm_proc_interrupt_109_handler();
extern "C" void asm_proc_interrupt_110_handler();
extern "C" void asm_proc_interrupt_111_handler();
extern "C" void asm_proc_interrupt_112_handler();
extern "C" void asm_proc_interrupt_113_handler();
extern "C" void asm_proc_interrupt_114_handler();
extern "C" void asm_proc_interrupt_115_handler();
extern "C" void asm_proc_interrupt_116_handler();
extern "C" void asm_proc_interrupt_117_handler();
extern "C" void asm_proc_interrupt_118_handler();
extern "C" void asm_proc_interrupt_119_handler();
extern "C" void asm_proc_interrupt_120_handler();
extern "C" void asm_proc_interrupt_121_handler();
extern "C" void asm_proc_interrupt_122_handler();
extern "C" void asm_proc_interrupt_123_handler();
extern "C" void asm_proc_interrupt_124_handler();
extern "C" void asm_proc_interrupt_125_handler();
extern "C" void asm_proc_interrupt_126_handler();
extern "C" void asm_proc_interrupt_127_handler();
extern "C" void asm_proc_interrupt_128_handler();
extern "C" void asm_proc_interrupt_129_handler();
extern "C" void asm_proc_interrupt_130_handler();
extern "C" void asm_proc_interrupt_131_handler();
extern "C" void asm_proc_interrupt_132_handler();
extern "C" void asm_proc_interrupt_133_handler();
extern "C" void asm_proc_interrupt_134_handler();
extern "C" void asm_proc_interrupt_135_handler();
extern "C" void asm_proc_interrupt_136_handler();
extern "C" void asm_proc_interrupt_137_handler();
extern "C" void asm_proc_interrupt_138_handler();
extern "C" void asm_proc_interrupt_139_handler();
extern "C" void asm_proc_interrupt_140_handler();
extern "C" void asm_proc_interrupt_141_handler();
extern "C" void asm_proc_interrupt_142_handler();
extern "C" void asm_proc_interrupt_143_handler();
extern "C" void asm_proc_interrupt_144_handler();
extern "C" void asm_proc_interrupt_145_handler();
extern "C" void asm_proc_interrupt_146_handler();
extern "C" void asm_proc_interrupt_147_handler();
extern "C" void asm_proc_interrupt_148_handler();
extern "C" void asm_proc_interrupt_149_handler();
extern "C" void asm_proc_interrupt_150_handler();
extern "C" void asm_proc_interrupt_151_handler();
extern "C" void asm_proc_interrupt_152_handler();
extern "C" void asm_proc_interrupt_153_handler();
extern "C" void asm_proc_interrupt_154_handler();
extern "C" void asm_proc_interrupt_155_handler();
extern "C" void asm_proc_interrupt_156_handler();
extern "C" void asm_proc_interrupt_157_handler();
extern "C" void asm_proc_interrupt_158_handler();
extern "C" void asm_proc_interrupt_159_handler();
extern "C" void asm_proc_interrupt_160_handler();
extern "C" void asm_proc_interrupt_161_handler();
extern "C" void asm_proc_interrupt_162_handler();
extern "C" void asm_proc_interrupt_163_handler();
extern "C" void asm_proc_interrupt_164_handler();
extern "C" void asm_proc_interrupt_165_handler();
extern "C" void asm_proc_interrupt_166_handler();
extern "C" void asm_proc_interrupt_167_handler();
extern "C" void asm_proc_interrupt_168_handler();
extern "C" void asm_proc_interrupt_169_handler();
extern "C" void asm_proc_interrupt_170_handler();
extern "C" void asm_proc_interrupt_171_handler();
extern "C" void asm_proc_interrupt_172_handler();
extern "C" void asm_proc_interrupt_173_handler();
extern "C" void asm_proc_interrupt_174_handler();
extern "C" void asm_proc_interrupt_175_handler();
extern "C" void asm_proc_interrupt_176_handler();
extern "C" void asm_proc_interrupt_177_handler();
extern "C" void asm_proc_interrupt_178_handler();
extern "C" void asm_proc_interrupt_179_handler();
extern "C" void asm_proc_interrupt_180_handler();
extern "C" void asm_proc_interrupt_181_handler();
extern "C" void asm_proc_interrupt_182_handler();
extern "C" void asm_proc_interrupt_183_handler();
extern "C" void asm_proc_interrupt_184_handler();
extern "C" void asm_proc_interrupt_185_handler();
extern "C" void asm_proc_interrupt_186_handler();
extern "C" void asm_proc_interrupt_187_handler();
extern "C" void asm_proc_interrupt_188_handler();
extern "C" void asm_proc_interrupt_189_handler();
extern "C" void asm_proc_interrupt_190_handler();
extern "C" void asm_proc_interrupt_191_handler();
extern "C" void asm_proc_interrupt_192_handler();
extern "C" void asm_proc_interrupt_193_handler();
extern "C" void asm_proc_interrupt_194_handler();
extern "C" void asm_proc_interrupt_195_handler();
extern "C" void asm_proc_interrupt_196_handler();
extern "C" void asm_proc_interrupt_197_handler();
extern "C" void asm_proc_interrupt_198_handler();
extern "C" void asm_proc_interrupt_199_handler();
extern "C" void asm_proc_interrupt_200_handler();
extern "C" void asm_proc_interrupt_201_handler();
extern "C" void asm_proc_interrupt_202_handler();
extern "C" void asm_proc_interrupt_203_handler();
extern "C" void asm_proc_interrupt_204_handler();
extern "C" void asm_proc_interrupt_205_handler();
extern "C" void asm_proc_interrupt_206_handler();
extern "C" void asm_proc_interrupt_207_handler();
extern "C" void asm_proc_interrupt_208_handler();
extern "C" void asm_proc_interrupt_209_handler();
extern "C" void asm_proc_interrupt_210_handler();
extern "C" void asm_proc_interrupt_211_handler();
extern "C" void asm_proc_interrupt_212_handler();
extern "C" void asm_proc_interrupt_213_handler();
extern "C" void asm_proc_interrupt_214_handler();
extern "C" void asm_proc_interrupt_215_handler();
extern "C" void asm_proc_interrupt_216_handler();
extern "C" void asm_proc_interrupt_217_handler();
extern "C" void asm_proc_interrupt_218_handler();
extern "C" void asm_proc_interrupt_219_handler();
extern "C" void asm_proc_interrupt_220_handler();
extern "C" void asm_proc_interrupt_221_handler();
extern "C" void asm_proc_interrupt_222_handler();
extern "C" void asm_proc_interrupt_223_handler();
extern "C" void asm_proc_interrupt_224_handler();
extern "C" void asm_proc_interrupt_225_handler();
extern "C" void asm_proc_interrupt_226_handler();
extern "C" void asm_proc_interrupt_227_handler();
extern "C" void asm_proc_interrupt_228_handler();
extern "C" void asm_proc_interrupt_229_handler();
extern "C" void asm_proc_interrupt_230_handler();
extern "C" void asm_proc_interrupt_231_handler();
extern "C" void asm_proc_interrupt_232_handler();
extern "C" void asm_proc_interrupt_233_handler();
extern "C" void asm_proc_interrupt_234_handler();
extern "C" void asm_proc_interrupt_235_handler();
extern "C" void asm_proc_interrupt_236_handler();
extern "C" void asm_proc_interrupt_237_handler();
extern "C" void asm_proc_interrupt_238_handler();
extern "C" void asm_proc_interrupt_239_handler();
extern "C" void asm_proc_interrupt_240_handler();
extern "C" void asm_proc_interrupt_241_handler();
extern "C" void asm_proc_interrupt_242_handler();
extern "C" void asm_proc_interrupt_243_handler();
extern "C" void asm_proc_interrupt_244_handler();
extern "C" void asm_proc_interrupt_245_handler();
extern "C" void asm_proc_interrupt_246_handler();
extern "C" void asm_proc_interrupt_247_handler();
extern "C" void asm_proc_interrupt_248_handler();
extern "C" void asm_proc_interrupt_249_handler();
extern "C" void asm_proc_interrupt_250_handler();
extern "C" void asm_proc_interrupt_251_handler();
extern "C" void asm_proc_interrupt_252_handler();
extern "C" void asm_proc_interrupt_253_handler();
extern "C" void asm_proc_interrupt_254_handler();
extern "C" void asm_proc_interrupt_255_handler();

/// @endcond
