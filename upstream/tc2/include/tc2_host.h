/* tc2_host.h -- what the lifted program's runtime (src/tc2_cpu.c) needs from the Time Crisis 2 host (src/tc2_mem.c, src/tc2_main.c) */
#ifndef TC2_HOST_H
#define TC2_HOST_H
#include <stdint.h>
/* Ghidra's MIPS register space (decompiled/pcode.txt "R" lines): 64-bit registers, the address in the low half */
#define TC2_R_SP      0xE8
#define TC2_R_COUNT   0x2048
#define TC2_R_COMPARE 0x2058
#define TC2_R_STATUS  0x2060
#define TC2_R_CAUSE   0x2068
#define TC2_R_EPC     0x2070
#define TC2_R_ERROREPC 0x20F0              /* CP0 30 */
#define TC2_SLICE     1000                 /* instructions between scheduler polls */
void tc2_host_step(uint32_t instructions);  /* time passed: devices, frames */
uint32_t tc2_until_event(void);            /* instructions to the next interrupt event: the scheduler polls right there */
void tc2_trace_mark(const char *what, uint32_t v);
void tc2_on_trap(void);
void tc2_on_exit(void);
void tc2_advance_count(uint32_t instructions);
void tc2_poll_irq(void);
void tc2_irq_set(int src, int on);          /* an interrupt SOURCE (TC2_SRC_*, include/tc2_game.h) on / off: the game's table picks its pin (src/tc2_cpu.c) */
int  tc2_irq_is(int src);                   /* that source pending */
uint32_t tc2_irq_sources(void);             /* all pending sources (bit = TC2_SRC_*) */
extern uint32_t rr_frame, rr_n_traps, rr_pc;
extern uint64_t rr_n_irq;
#endif
