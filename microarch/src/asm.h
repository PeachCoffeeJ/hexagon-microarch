#include <inttypes.h>

static inline __attribute__((always_inline)) uint64_t rdtsc(void) {
    uint64_t processor_cycles;
    asm volatile("%0 = UPCYCLE\n" : "=r" (processor_cycles));
    return processor_cycles;
}

static inline __attribute__((always_inline)) uint32_t read_user_reg(void) {
    uint32_t usr_reg;
    asm volatile("%0 = USR\n" : "=r" (usr_reg));
    return usr_reg;
}

static inline __attribute__((always_inline)) void barrier(void) {
    asm volatile("barrier\n");
}

static inline __attribute__((always_inline)) void syncht(void) {
    asm volatile("syncht\n");
}

static inline __attribute__((always_inline)) void nop(void) {
    asm volatile(
        ".rept 30\n"
        "nop\n"
        ".endr\n"
    );
}

static inline __attribute__((always_inline)) void one_cache_miss(unsigned char *reloadbuffer) {
    asm volatile(
    "{ r0 = memub(%0) }\n"
    ::"r"(reloadbuffer) : "r0"
    );
}

static inline __attribute__((always_inline)) void two_cache_miss(unsigned char *reloadbuffer1, unsigned char *reloadbuffer2) {
    asm volatile(
    "{ r1 = memub(%0) ; r2 = memub(%1) }\n"
    // "nop\n"
    // "nop\n"
    ::"r"(reloadbuffer1), "r"(reloadbuffer2) : "r1", "r2"
    );
}

static inline __attribute__((always_inline)) void alu_ins(void) {
    asm volatile(
        ".rept 50\n"
        "r0 = asl(r0,#9)\n"
        ".endr\n"
        ::: "r0"
    );
}

static inline __attribute__((always_inline)) void maccess(unsigned char *reloadbuffer, uint8_t offset) {
    asm volatile(
    "r0 = %1\n"
    "r0 = asl(r0,#12)\n"
    "r0 = add(r0,%0)\n"
    "r0 = memub(r0)\n"
    ::"r"(reloadbuffer), "r"(offset) : "r0"
    );
}

static inline __attribute__((always_inline)) uint64_t flush_reload(uint8_t *reloadbuffer) {
    uint64_t start_time;
    uint64_t end_time;
    uint64_t access_time;
    asm volatile(
        "barrier\n"
        "%0 = UPCYCLE\n"
        "r0 = memub(%2)\n"
        "%1 = UPCYCLE\n"
        "dccleaninva(%2)\n"
    :"=r" (start_time), "=r" (end_time)
    :"r"(reloadbuffer) :"r0"
    );
    access_time = end_time - start_time;
    return access_time;
}

// static inline __attribute__((always_inline)) uint32_t conditional_branch_test(unsigned char *reloadbuffer, unsigned char *target_offset, unsigned char offset) {
//     uint32_t flag = 0;
//     asm volatile(
//     // "{\n"
//     // "r1 = %1\n"
//     // "r2 = %2\n"
//     "r0 = %0\n"
//     "r3 = asl(%3,#7)\n"
//     "r1 = add(r3,%1)\n"
//     // "}\n"
//     /*
//         if (*target_offset > offset) {
//             access_char = reloadbuffer[offset << 12];
//         }
//     */
//     "{\n"
//     "r4 = memub(%2)\n"
//     // "r4 = #1\n"
//     "P1 = cmpb.gtu(r4,%3)\n"
//     "if (P1.new) r5 = memub(r1)\n"
//     // "if (P1.new) r5 = r4\n"
//     // "r3 = memub(r0)\n"
//     "}\n"
//     :"=r" (flag)
//     :"r"(reloadbuffer), "r"(target_offset), "r"(offset) : "r0", "r1", "r2", "r3", "r4", "r5"
//     );
//     return flag;
// }

static inline __attribute__((always_inline)) uint32_t conditional_branch_test(unsigned char *reloadbuffer, unsigned char *target_offset, unsigned char offset) {
    uint32_t flag = 0;
    asm volatile(
    // "{\n"
    // "r1 = %1\n"
    "r2 = %3\n"
    "r0 = %0\n"
    "r3 = asl(%3,#7)\n"
    "r1 = add(r3,%1)\n"
    // "}\n"
    /*
        if (*target_offset > offset) {
            access_char = reloadbuffer[offset << 12];
        }
    */
    "{\n"
    "r4 = memub(%2)\n"
    // "r4 = #1\n"
    "P1 = cmpb.gtu(r4,r2)\n"
    "if (P1.new) r5 = memub(r1)\n"
    "if (P1.new) jump:t KTEST\n"
    // "if (P1.new) %0 = add(r0,#1)\n"
    // "r3 = memub(r0)\n"
    "}\n"
    "%0 = #0\n"
    "jump KEXIT\n"
    "KTEST:\n"
    "%0 = #1\n"
    "KEXIT:\n"
    :"=r" (flag)
    :"r"(reloadbuffer), "r"(target_offset), "r"(offset) : "r0", "r1", "r2", "r3", "r4", "r5"
    );
    // flag = 1;
    return flag;
}

// static inline __attribute__((always_inline)) void conditional_branch(unsigned char *reloadbuffer, uint8_t *secret, uint32_t target_offset, uint32_t offset) {
//     asm volatile(
//     "r3 = %3\n"
//     // "r4 = #0x00ffffff\n"
//     /*
//         if ((target_offset << 24) < offset) {
//             access_char = reloadbuffer[*secret * 128];
//         }
//     */
//     "{\n"
//     "r4 = asl(%2,#24)\n"
//     "P0 = cmp.gtu(r3,r4)\n"
//     "if (P0.new) r5 = memub(%1)\n"
//     "if (P0.new) jump:t TAKEN\n"
//     "}\n"
//     "jump NTAKEN\n"
//     "TAKEN:\n"
//     // "{\n"
    
//     "r6 = add(%0,mpyi(#128,r5))\n"
//     "r7 = memub(r6)\n"
//     // "}\n"
//     "NTAKEN:\n"
//     :: "r"(reloadbuffer), "r"(secret), "r"(target_offset), "r"(offset) : /*"r0", "r1", "r2", */"r3", "r4", "r5", "r6", "r7"
//     );
// }

static inline __attribute__((always_inline)) uint8_t exe_path_test(unsigned char *reloadbuffer, uint8_t *secret, uint32_t target_offset, uint32_t offset) {
    uint8_t flag = 0;
    asm volatile(
    // "r2 = #1\n"
    "r2 = %3\n"
    // "r3 = #0x0f\n"
    "r3 = %4\n"
    // "r5 = #0\n"
    "{\n"
    "r4 = asl(r2,#24)\n"
    "P0 = cmp.gtu(r3,r4)\n"
    // "if (!P0.new) r5 = memub(%1)\n"
    "if (P0.new) jump:t TAKEN\n"
    "if (!P0.new) jump:nt NTAKEN\n"
    "}\n"
    // "jump NTAKEN\n"


    "TAKEN:\n"
    // "{\n"
    // "r5 = memub(%2)\n"
    // "r6 = add(%1,mpyi(#128,r5))\n"
    // "r7 = memub(r6)\n"
    "%0 = #0xff\n"
    "jump EXIT\n"
    // "}\n"

    "NTAKEN:\n"
    // "{\n"
    // "r5 = memub(%2)\n"
    // "r6 = add(%1,mpyi(#128,r5))\n"
    // "r7 = memub(r6)\n"
    "%0 = #0\n"
    // "}\n"

    "EXIT:\n"
    : "=r" (flag)
    :"r"(reloadbuffer), "r"(secret), "r"(target_offset), "r"(offset) : /*"r0", "r1", */"r2", "r3", "r4", "r5", "r6", "r7"
    );
    return flag;
}

// static inline __attribute__((always_inline)) void conditional_branch(uint8_t *reloadbuffer, uint8_t *secret, uint8_t *target_offset, uint32_t offset) {
//     asm volatile(
//     // "r2 = #0\n"
//     /*
//         if ((*target_offset << 24) < offset) {
//             access_char = reloadbuffer[*secret * 128];
//         }
//     */
//     "{\n"
//     "r3 = memub(%2)\n"
//     "r4 = asl(r3,#24)\n"
//     "P0 = cmp.ltu(r4,%3)\n"
//     "if (P0.new) jump:t TAKEN\n"
//     "}\n"
//     "jump NTAKEN\n"
//     "TAKEN:\n"
//     // "{\n"
//     "r5 = memub(%1)\n"
//     "r6 = add(%0,mpyi(#128,r5))\n"
//     "r7 = memub(r6)\n"
//     // "}\n"
//     "NTAKEN:\n"
//     ::"r"(reloadbuffer), "r"(secret), "r"(target_offset), "r"(offset) : /*"r0", "r1", "r2", */"r3", "r4", "r5", "r6", "r7"
//     );
// }
