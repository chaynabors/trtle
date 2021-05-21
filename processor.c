#include "processor.h"

#include "gameboy.h"
#include "interrupt_controller.h"
#include "logger.h"

#define INTERRUPT_FLAGS_ADDRESS  (0xFF0F)
#define INTERRUPT_ENABLE_ADDRESS (0xFFFF)

#define PROCESSOR_CARRY_BIT      (0b00010000)
#define PROCESSOR_HALF_BIT       (0b00100000)
#define PROCESSOR_NEGATIVE_BIT   (0b01000000)
#define PROCESSOR_ZERO_BIT       (0b10000000)

void processor_initialize(Processor * const p, bool skip_bootrom) {
    p->af = 0x01B0;
    p->bc = 0x0013;
    p->de = 0x00D8;
    p->hl = 0x014D;
    p->sp = 0xFFFE;
    p->pc = skip_bootrom ? 0x0100 : 0x0000;
    p->halt_mode = false;
    p->skip_pc_increment = false;
    p->skip_next_interrupt = false;
}

#define LD_R_R(reg1, reg2)\
static void ld_##reg1##_##reg2(GameBoy * const gb) {\
    gb->processor->reg1 = gb->processor->reg2;\
}

#define LD_R_D8(reg)\
static void ld_##reg##_d8(GameBoy * const gb) {\
    gb->processor->reg = gameboy_read(gb, gb->processor->pc);\
    gb->processor->pc += 1;\
    gameboy_cycle(gb);\
}

#define LD_R_DHL(reg)\
static void ld_##reg##_dhl(GameBoy * const gb) {\
    gb->processor->reg = gameboy_read(gb, gb->processor->hl);\
    gameboy_cycle(gb);\
}

#define LD_DHL_R(reg)\
static void ld_dhl_##reg(GameBoy * const gb) {\
    gameboy_write(gb, gb->processor->hl, gb->processor->reg);\
    gameboy_cycle(gb);\
}

static void ld_dhl_d8(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);
}

#define LD_A_DRR(reg)\
static void ld_a_d##reg(GameBoy * const gb) {\
    gb->processor->a = gameboy_read(gb, gb->processor->reg);\
    gameboy_cycle(gb);\
}

#define LD_DRR_A(reg)\
static void ld_d##reg##_a(GameBoy * const gb) {\
    gameboy_write(gb, gb->processor->reg, gb->processor->a);\
    gameboy_cycle(gb);\
}

static void ld_a_da16(GameBoy * const gb) {
    uint16_t addr = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);
    addr |= gameboy_read(gb, gb->processor->pc) << 8;
    gb->processor->pc += 1;
    gameboy_cycle(gb);
    gb->processor->a = gameboy_read(gb, addr);
    gameboy_cycle(gb);
}

static void ld_da16_a(GameBoy * const gb) {
    uint16_t addr = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);
    addr |= gameboy_read(gb, gb->processor->pc) << 8;
    gb->processor->pc += 1;
    gameboy_cycle(gb);
    gameboy_write(gb, addr, gb->processor->a);
    gameboy_cycle(gb);
}

static void ldh_a_dc(GameBoy * const gb) {
    gb->processor->a = gameboy_read(gb, 0xFF00 | gb->processor->c);
    gameboy_cycle(gb);
}

static void ldh_dc_a(GameBoy * const gb) {
    gameboy_write(gb, 0xFF00 | gb->processor->c, gb->processor->a);
    gameboy_cycle(gb);
}

static void ldh_a_da8(GameBoy * const gb) {
    uint16_t addr = 0xFF00 | gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    gb->processor->a = gameboy_read(gb, addr);
    gameboy_cycle(gb);
}

static void ldh_da8_a(GameBoy * const gb) {
    uint16_t addr = 0xFF00 | gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    gameboy_write(gb, addr, gb->processor->a);
    gameboy_cycle(gb);
}

static void ld_a_dhld(GameBoy * const gb) {
    gb->processor->a = gameboy_read(gb, gb->processor->hl);
    gb->processor->hl -= 1;
    gameboy_cycle(gb);
}

static void ld_dhld_a(GameBoy * const gb) {
    gameboy_write(gb, gb->processor->hl, gb->processor->a);
    gb->processor->hl -= 1;
    gameboy_cycle(gb);
}

static void ld_a_dhli(GameBoy * const gb) {
    gb->processor->a = gameboy_read(gb, gb->processor->hl);
    gb->processor->hl += 1;
    gameboy_cycle(gb);
}

static void ld_dhli_a(GameBoy * const gb) {
    gameboy_write(gb, gb->processor->hl, gb->processor->a);
    gb->processor->hl += 1;
    gameboy_cycle(gb);
}

#define LD_RR_D16(reg)\
static void ld_##reg##_d16(GameBoy * const gb) {\
    uint16_t val = gameboy_read(gb, gb->processor->pc++);\
    gameboy_cycle(gb);\
    val |= gameboy_read(gb, gb->processor->pc++) << 8;\
    gameboy_cycle(gb);\
    gb->processor->reg = val;\
}

static void ld_da16_sp(GameBoy * const gb) {
    uint16_t addr = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    addr |= gameboy_read(gb, gb->processor->pc++) << 8;
    gameboy_cycle(gb);
    gameboy_write(gb, addr, (gb->processor->sp) & 0x00FF);
    gameboy_cycle(gb);
    gameboy_write(gb, addr + 1, (gb->processor->sp) >> 8);
    gameboy_cycle(gb);
}

static void ld_sp_hl(GameBoy * const gb) {
    gb->processor->sp = gb->processor->hl;
    gameboy_cycle(gb);
}

#define PUSH_RR(reg)\
static void push_##reg(GameBoy * const gb) {\
    gameboy_cycle(gb);\
    gameboy_write(gb, --gb->processor->sp, gb->processor->reg >> 8);\
    gameboy_cycle(gb);\
    gameboy_write(gb, --gb->processor->sp, gb->processor->reg & 0x00FF);\
    gameboy_cycle(gb);\
}

#define POP_RR(reg)\
static void pop_##reg(GameBoy * const gb) {\
    uint16_t val = gameboy_read(gb, gb->processor->sp++);\
    gameboy_cycle(gb);\
    val |= gameboy_read(gb, gb->processor->sp++) << 8;\
    gameboy_cycle(gb);\
    gb->processor->reg = val;\
}

static void pop_af(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->sp++);
    gameboy_cycle(gb);
    val |= gameboy_read(gb, gb->processor->sp++) << 8;
    gameboy_cycle(gb);
    gb->processor->af = val & 0xFFF0;
}

#define INC_R(reg)\
static void inc_##reg(GameBoy * const gb) {\
    gb->processor->reg += 1;\
\
    gb->processor->f &= PROCESSOR_CARRY_BIT;\
    if ((gb->processor->reg & 0x0F) == 0) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

#define DEC_R(reg)\
static void dec_##reg(GameBoy * const gb) {\
    gb->processor->reg -= 1;\
\
    gb->processor->f &= PROCESSOR_CARRY_BIT;\
    gb->processor->f |= PROCESSOR_NEGATIVE_BIT;\
    if ((gb->processor->reg & 0x0F) == 0x0F) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

#define ADD_A_R(reg)\
static void add_a_##reg(GameBoy * const gb) {\
    uint8_t initial = gb->processor->a;\
    uint8_t add = gb->processor->reg;\
\
    gb->processor->a += add;\
\
    gb->processor->f = 0;\
    if (((initial & 0x0F) + (add & 0x0F)) & 0x10) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
    if (((uint16_t)initial) + ((uint16_t)add) > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;\
}

static void add_a_dhl(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t add = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    gb->processor->a += add;

    gb->processor->f = 0;
    if (((initial & 0x0F) + (add & 0x0F)) & 0x10) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (((uint16_t)initial) + ((uint16_t)add) > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void add_a_d8(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t add = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);

    gb->processor->a += add;

    gb->processor->f = 0;
    if (((initial & 0x0F) + (add & 0x0F)) & 0x10) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (((uint16_t)initial) + ((uint16_t)add) > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

#define ADC_A_R(reg)\
static void adc_a_##reg(GameBoy * const gb) {\
    uint8_t initial = gb->processor->a;\
    uint8_t add = gb->processor->reg;\
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;\
\
    gb->processor->a += add + car;\
\
    gb->processor->f = 0;\
    if (((initial & 0x0F) + (add & 0x0F) + (car & 0x0F)) & 0x10) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
    if (((uint16_t)initial) + ((uint16_t)add) + ((uint16_t)car) > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;\
}

static void adc_a_dhl(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t add = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;

    gb->processor->a += add + car;

    gb->processor->f = 0;
    if (((initial & 0x0F) + (add & 0x0F) + (car & 0x0F)) & 0x10) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (((uint16_t)initial) + ((uint16_t)add) + ((uint16_t)car) > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void adc_a_d8(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t add = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;

    gb->processor->a += add + car;

    gb->processor->f = 0;
    if (((initial & 0x0F) + (add & 0x0F) + (car & 0x0F)) & 0x10) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (((uint16_t)initial) + ((uint16_t)add) + ((uint16_t)car) > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

#define SUB_A_R(reg)\
static void sub_a_##reg(GameBoy * const gb) {\
    uint8_t initial = gb->processor->a;\
    uint8_t sub = gb->processor->reg;\
\
    gb->processor->a -= sub;\
\
    gb->processor->f = PROCESSOR_NEGATIVE_BIT;\
    if ((initial & 0x0F) < (sub & 0x0F)) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
    if (initial < sub) gb->processor->f |= PROCESSOR_CARRY_BIT;\
}

static void sub_a_dhl(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t sub = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    gb->processor->a -= sub;

    gb->processor->f = PROCESSOR_NEGATIVE_BIT;
    if ((initial & 0x0F) < (sub & 0x0F)) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (initial < sub) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void sub_a_d8(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t sub = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);

    gb->processor->a -= sub;

    gb->processor->f = PROCESSOR_NEGATIVE_BIT;
    if ((initial & 0x0F) < (sub & 0x0F)) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (initial < sub) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

#define SBC_A_R(reg)\
static void sbc_a_##reg(GameBoy * const gb) {\
    uint8_t initial = gb->processor->a;\
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;\
\
    gb->processor->a = initial - gb->processor->reg - car;\
\
    gb->processor->f = PROCESSOR_NEGATIVE_BIT;\
    if ((initial & 0x0F) < (gb->processor->reg & 0x0F) + car) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
    if (((uint32_t)initial) - gb->processor->reg - car > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;\
}

static void sbc_a_dhl(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t sub = gameboy_read(gb, gb->processor->hl);
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;
    gameboy_cycle(gb);

    gb->processor->a = initial - sub - car;

    gb->processor->f = PROCESSOR_NEGATIVE_BIT;
    if ((initial & 0x0F) < (sub & 0x0F) + car) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (((uint32_t)initial) - sub - car > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void sbc_a_d8(GameBoy * const gb) {
    uint8_t initial = gb->processor->a;
    uint8_t sub = gameboy_read(gb, gb->processor->pc++);
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;
    gameboy_cycle(gb);

    gb->processor->a = initial - sub - car;

    gb->processor->f = PROCESSOR_NEGATIVE_BIT;
    if ((initial & 0x0F) < (sub & 0x0F) + car) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (((uint32_t)initial) - sub - car > 0xFF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

#define AND_A_R(reg)\
static void and_a_##reg(GameBoy * const gb) {\
    gb->processor->a &= gb->processor->reg;\
\
    gb->processor->f = PROCESSOR_HALF_BIT;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void and_a_dhl(GameBoy * const gb) {
    gb->processor->a &= gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    gb->processor->f = PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

static void and_a_d8(GameBoy * const gb) {
    gb->processor->a &= gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);

    gb->processor->f = PROCESSOR_HALF_BIT;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define XOR_A_R(reg)\
static void xor_a_##reg(GameBoy * const gb) {\
    gb->processor->a ^= gb->processor->reg;\
\
    gb->processor->f = 0x00;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void xor_a_dhl(GameBoy * const gb) {
    gb->processor->a ^= gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    gb->processor->f = 0x00;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

static void xor_a_d8(GameBoy * const gb) {
    gb->processor->a ^= gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);

    gb->processor->f = 0x00;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define OR_A_R(reg)\
static void or_a_##reg(GameBoy * const gb) {\
    gb->processor->a |= gb->processor->reg;\
\
    gb->processor->f = 0x00;\
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void or_a_dhl(GameBoy * const gb) {
    gb->processor->a |= gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    gb->processor->f = 0x00;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

static void or_a_d8(GameBoy * const gb) {
    gb->processor->a |= gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);

    gb->processor->f = 0x00;
    if (gb->processor->a == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define CP_A_R(reg)\
static void cp_a_##reg(GameBoy * const gb) {\
    gb->processor->f = PROCESSOR_NEGATIVE_BIT;\
\
    if ((gb->processor->a & 0x0F) < (gb->processor->reg & 0x0F)) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (gb->processor->a == gb->processor->reg) gb->processor->f |= PROCESSOR_ZERO_BIT;\
    if (gb->processor->a < gb->processor->reg) gb->processor->f |= PROCESSOR_CARRY_BIT;\
}

static void cp_a_dhl(GameBoy * const gb) {
    uint8_t num = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    gb->processor->f = PROCESSOR_NEGATIVE_BIT;
    if ((gb->processor->a & 0x0F) < (num & 0x0F)) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == num) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (gb->processor->a < num) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void cp_a_d8(GameBoy * const gb) {
    uint8_t num = gameboy_read(gb, gb->processor->pc);
    gb->processor->pc += 1;
    gameboy_cycle(gb);

    gb->processor->f = PROCESSOR_NEGATIVE_BIT;
    if ((gb->processor->a & 0x0F) < (num & 0x0F)) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (gb->processor->a == num) gb->processor->f |= PROCESSOR_ZERO_BIT;
    if (gb->processor->a < num) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void inc_dhl(GameBoy * const gb) {
    uint8_t num = gameboy_read(gb, gb->processor->hl) + 1;
    gameboy_cycle(gb);
    gameboy_write(gb, gb->processor->hl, num);
    gameboy_cycle(gb);

    gb->processor->f &= PROCESSOR_CARRY_BIT;
    if ((num & 0x0F) == 0) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (num == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

static void dec_dhl(GameBoy * const gb) {
    uint8_t num = gameboy_read(gb, gb->processor->hl) - 1;
    gameboy_cycle(gb);
    gameboy_write(gb, gb->processor->hl, num);
    gameboy_cycle(gb);

    gb->processor->f &= PROCESSOR_CARRY_BIT;
    gb->processor->f |= PROCESSOR_NEGATIVE_BIT;
    if ((num & 0x0F) == 0x0F) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (num == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define INC_RR(reg)\
static void inc_##reg(GameBoy * const gb) {\
    gb->processor->reg += 1;\
    gameboy_cycle(gb);\
}

#define DEC_RR(reg)\
static void dec_##reg(GameBoy * const gb) {\
    gb->processor->reg -= 1;\
    gameboy_cycle(gb);\
}

#define ADD_HL_RR(reg)\
static void add_hl_##reg(GameBoy * const gb) {\
    uint16_t initial = gb->processor->hl;\
    uint16_t add = gb->processor->reg;\
    gameboy_cycle(gb);\
\
    gb->processor->hl += add;\
\
    gb->processor->f &= PROCESSOR_ZERO_BIT;\
    if (((initial & 0x0FFF) + (add & 0x0FFF)) & 0x1000) gb->processor->f |= PROCESSOR_HALF_BIT;\
    if ((((uint32_t)initial) + ((uint32_t)add)) & 0x10000) gb->processor->f |= PROCESSOR_CARRY_BIT;\
}

static void add_sp_r8(GameBoy * const gb) {
    uint16_t initial = gb->processor->sp;
    int8_t add = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    gameboy_cycle(gb);
    gameboy_cycle(gb);

    gb->processor->sp += add;

    gb->processor->f = 0;
    if (((initial & 0x000F) + (add & 0x000F)) > 0x000F) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (((initial & 0x00FF) + (add & 0x00FF)) > 0x00FF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void ld_hl_sp_r8(GameBoy * const gb) {
    uint16_t initial = gb->processor->sp;
    int8_t add = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    gameboy_cycle(gb);

    gb->processor->hl = initial + add;

    gb->processor->f = 0;
    if (((initial & 0x000F) + (add & 0x000F)) > 0x000F) gb->processor->f |= PROCESSOR_HALF_BIT;
    if (((initial & 0x00FF) + (add & 0x00FF)) > 0x00FF) gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void jp_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);
    gameboy_cycle(gb);

    gb->processor->pc = val;
}

static void jp_hl(GameBoy * const gb) {
    gb->processor->pc = gb->processor->hl;
}

static void jp_nz_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_ZERO_BIT) == 0) {
        gb->processor->pc = val;
        gameboy_cycle(gb);
    }
}

static void jp_z_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_ZERO_BIT) != 0) {
        gb->processor->pc = val;
        gameboy_cycle(gb);
    }
}

static void jp_nc_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_CARRY_BIT) == 0) {
        gb->processor->pc = val;
        gameboy_cycle(gb);
    }
}

static void jp_c_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_CARRY_BIT) != 0) {
        gb->processor->pc = val;
        gameboy_cycle(gb);
    }
}

static void jr_r8(GameBoy * const gb) {
    int8_t jump = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    gb->processor->pc += jump;
    gameboy_cycle(gb);
}

static void jr_nz_r8(GameBoy * const gb) {
    int8_t jump = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    if ((gb->processor->f & PROCESSOR_ZERO_BIT) == 0) {
        gb->processor->pc += jump;
        gameboy_cycle(gb);
    }
}

static void jr_z_r8(GameBoy * const gb) {
    int8_t jump = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    if ((gb->processor->f & PROCESSOR_ZERO_BIT) != 0) {
        gb->processor->pc += jump;
        gameboy_cycle(gb);
    }
}

static void jr_nc_r8(GameBoy * const gb) {
    int8_t jump = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    if ((gb->processor->f & PROCESSOR_CARRY_BIT) == 0) {
        gb->processor->pc += jump;
        gameboy_cycle(gb);
    }
}

static void jr_c_r8(GameBoy * const gb) {
    int8_t jump = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    if ((gb->processor->f & PROCESSOR_CARRY_BIT) != 0) {
        gb->processor->pc += jump;
        gameboy_cycle(gb);
    }
}

static void call_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);
    gameboy_cycle(gb); // Delay
    gameboy_write(gb, --gb->processor->sp, gb->processor->pc >> 8);
    gameboy_cycle(gb);
    gameboy_write(gb, --gb->processor->sp, gb->processor->pc & 0xFF);
    gameboy_cycle(gb);
    gb->processor->pc = val;
}

static void call_nz_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_ZERO_BIT) == 0) {
        gameboy_cycle(gb); // Delay
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc >> 8);
        gameboy_cycle(gb);
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc & 0xFF);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
}

static void call_z_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_ZERO_BIT) != 0) {
        gameboy_cycle(gb); // Delay
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc >> 8);
        gameboy_cycle(gb);
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc & 0xFF);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
}

static void call_nc_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_CARRY_BIT) == 0) {
        gameboy_cycle(gb); // Delay
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc >> 8);
        gameboy_cycle(gb);
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc & 0xFF);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
}

static void call_c_a16(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->pc++) << 8);
    gameboy_cycle(gb);

    if ((gb->processor->f & PROCESSOR_CARRY_BIT) != 0) {
        gameboy_cycle(gb); // Delay
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc >> 8);
        gameboy_cycle(gb);
        gameboy_write(gb, --gb->processor->sp, gb->processor->pc & 0xFF);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
}

static void ret(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->sp++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->sp++) << 8);
    gameboy_cycle(gb);
    gb->processor->pc = val;
    gameboy_cycle(gb); // Delay
}

static void ret_nz(GameBoy * const gb) {
    if ((gb->processor->f & PROCESSOR_ZERO_BIT) == 0) {
        gameboy_cycle(gb); //Delay
        uint16_t val = gameboy_read(gb, gb->processor->sp++);
        gameboy_cycle(gb);
        val |= (gameboy_read(gb, gb->processor->sp++) << 8);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
    gameboy_cycle(gb); // Delay
}

static void ret_z(GameBoy * const gb) {
    if ((gb->processor->f & PROCESSOR_ZERO_BIT) != 0) {
        gameboy_cycle(gb); //Delay
        uint16_t val = gameboy_read(gb, gb->processor->sp++);
        gameboy_cycle(gb);
        val |= (gameboy_read(gb, gb->processor->sp++) << 8);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
    gameboy_cycle(gb); // Delay
}

static void ret_nc(GameBoy * const gb) {
    if ((gb->processor->f & PROCESSOR_CARRY_BIT) == 0) {
        gameboy_cycle(gb); //Delay
        uint16_t val = gameboy_read(gb, gb->processor->sp++);
        gameboy_cycle(gb);
        val |= (gameboy_read(gb, gb->processor->sp++) << 8);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
    gameboy_cycle(gb); // Delay
}

static void ret_c(GameBoy * const gb) {
    if ((gb->processor->f & PROCESSOR_CARRY_BIT) != 0) {
        gameboy_cycle(gb); //Delay
        uint16_t val = gameboy_read(gb, gb->processor->sp++);
        gameboy_cycle(gb);
        val |= (gameboy_read(gb, gb->processor->sp++) << 8);
        gameboy_cycle(gb);
        gb->processor->pc = val;
    }
    gameboy_cycle(gb); // Delay
}

static void reti(GameBoy * const gb) {
    uint16_t val = gameboy_read(gb, gb->processor->sp++);
    gameboy_cycle(gb);
    val |= (gameboy_read(gb, gb->processor->sp++) << 8);
    gameboy_cycle(gb);
    gb->processor->pc = val;
    gb->interrupt_controller->ime = true;
    gameboy_cycle(gb); // Delay
}

#define RST_NNH(value)\
static void rst_##value##h(GameBoy * const gb) {\
    gameboy_cycle(gb);\
    gameboy_write(gb, --gb->processor->sp, gb->processor->pc >> 8);\
    gameboy_cycle(gb);\
    gameboy_write(gb, --gb->processor->sp, gb->processor->pc & 0x00FF);\
    gameboy_cycle(gb);\
\
    gb->processor->pc = 0x00##value;\
}

static void halt(GameBoy * const gb) {
    if (gb->interrupt_controller->ime == 0) {
        uint8_t interrupt_flags = gameboy_read(gb, INTERRUPT_FLAGS_ADDRESS);
        uint8_t interrupt_enables = gameboy_read(gb, INTERRUPT_ENABLE_ADDRESS);
        if ((interrupt_flags & interrupt_enables & 0x1F) == 0) gb->processor->skip_next_interrupt = true;
        else {
            gb->processor->skip_pc_increment = true;
            return;
        }
    }
    gb->processor->halt_mode = true;
}

static void stop(GameBoy * const gb) {
    TRTLE_LOG_ERR("Stop called and not implemented\n");
}

static void di(GameBoy * const gb) {
    gb->interrupt_controller->ime = false;
}

static void ei(GameBoy * const gb) {
    if (gb->interrupt_controller->ime == false) gb->interrupt_controller->cycles_until_ime = 1;
}

static void ccf(GameBoy * const gb) {
    uint8_t zero_bit = gb->processor->f & PROCESSOR_ZERO_BIT;
    gb->processor->f ^= PROCESSOR_CARRY_BIT;
    gb->processor->f &= PROCESSOR_CARRY_BIT;
    gb->processor->f |= zero_bit;
}

static void scf(GameBoy * const gb) {
    gb->processor->f &= PROCESSOR_ZERO_BIT;
    gb->processor->f |= PROCESSOR_CARRY_BIT;
}

static void nop(GameBoy * const gb) {}

static void inv_op(GameBoy * const gb) {
    TRTLE_LOG_WARN("Attempted to execute an invalid opcode\n");
}

static void daa(GameBoy * const gb) {
    if ((gb->processor->f & PROCESSOR_NEGATIVE_BIT) == 0) {
        if ((gb->processor->f & PROCESSOR_CARRY_BIT) != 0 || gb->processor->a > 0x99) {
            gb->processor->a += 0x60;
            gb->processor->f |= PROCESSOR_CARRY_BIT;
        }
        if ((gb->processor->f & PROCESSOR_HALF_BIT) != 0 || (gb->processor->a & 0x0f) > 0x09) {
            gb->processor->a += 0x6;
        }
    }
    else {
        if ((gb->processor->f & PROCESSOR_CARRY_BIT) != 0) gb->processor->a -= 0x60;
        if ((gb->processor->f & PROCESSOR_HALF_BIT) != 0) gb->processor->a -= 0x6;
    }
    gb->processor->f ^= (-(gb->processor->a == 0) ^ gb->processor->f) & PROCESSOR_ZERO_BIT;
    gb->processor->f &= ~PROCESSOR_HALF_BIT;
}

static void cpl(GameBoy * const gb) {
    gb->processor->a ^= 0xFF;
    gb->processor->f |= PROCESSOR_NEGATIVE_BIT | PROCESSOR_HALF_BIT;
}

static void rlca(GameBoy * const gb) {
    uint8_t car = (gb->processor->a & 0x80) != 0;
    gb->processor->f = 0;
    gb->processor->a <<= 1;
    if (car) {
        gb->processor->f |= PROCESSOR_CARRY_BIT;
        gb->processor->a |= 0x01;
    }
}

static void rrca(GameBoy * const gb) {
    uint8_t car = (gb->processor->a & 0x01) != 0;
    gb->processor->f = 0;
    gb->processor->a >>= 1;
    if (car) {
        gb->processor->f |= PROCESSOR_CARRY_BIT;
        gb->processor->a |= 0x80;
    }
}

static void rla(GameBoy * const gb) {
    uint8_t top = (gb->processor->a & 0x80) != 0;
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;
    gb->processor->f = 0;
    gb->processor->a <<= 1;
    if (top) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (car) gb->processor->a |= 0x01;
}

static void rra(GameBoy * const gb) {
    uint8_t bot = (gb->processor->a & 0x01) != 0;
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;
    gb->processor->f = 0;
    gb->processor->a >>= 1;
    if (bot) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (car) gb->processor->a |= 0x80;
}

#define RLC_R(reg)\
static void rlc_##reg(GameBoy * const gb) {\
    uint8_t car = (gb->processor->reg & 0x80) != 0;\
    gb->processor->reg = (gb->processor->reg << 1) | car;\
\
    gb->processor->f = 0;\
    if (car) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void rlc_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    uint8_t car = (val & 0x80) != 0;
    val = (val << 1) | car;
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);

    gb->processor->f = 0;
    if (car) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define RRC_R(reg)\
static void rrc_##reg(GameBoy * const gb) {\
    uint8_t car = (gb->processor->reg & 0x01) != 0;\
    gb->processor->reg = (gb->processor->reg >> 1) | (car << 7);\
\
    gb->processor->f = 0;\
    if (car) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void rrc_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    uint8_t car = (val & 0x01) != 0;
    val = (val >> 1) | (car << 7);
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);

    gb->processor->f = 0;
    if (car) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define RL_R(reg)\
static void rl_##reg(GameBoy * const gb) {\
    uint8_t top = (gb->processor->reg & 0x80) != 0;\
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;\
    gb->processor->reg = (gb->processor->reg << 1) | car;\
\
    gb->processor->f = 0;\
    if (top) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void rl_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    uint8_t top = (val & 0x80) != 0;
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;
    val = (val << 1) | car;
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);

    gb->processor->f = 0;
    if (top) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define RR_R(reg)\
static void rr_##reg(GameBoy * const gb) {\
    uint8_t bot = (gb->processor->reg & 0x01) != 0;\
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;\
    gb->processor->reg = (gb->processor->reg >> 1) | (car << 7);\
\
    gb->processor->f = 0;\
    if (bot) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void rr_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    uint8_t bot = (val & 0x01) != 0;
    uint8_t car = (gb->processor->f & PROCESSOR_CARRY_BIT) != 0;
    val = (val >> 1) | (car << 7);
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);

    gb->processor->f = 0;
    if (bot) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define SLA_R(reg)\
static void sla_##reg(GameBoy * const gb) {\
    uint8_t car = (gb->processor->reg & 0x80) != 0;\
    gb->processor->reg <<= 1;\
\
    gb->processor->f = 0;\
    if (car) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void sla_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);

    uint8_t car = (val & 0x80) != 0;
    val <<= 1;
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);

    gb->processor->f = 0;
    if (car) gb->processor->f |= PROCESSOR_CARRY_BIT;
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define SRA_R(reg)\
static void sra_##reg(GameBoy * const gb) {\
    uint8_t top = gb->processor->reg & 0x80;\
    gb->processor->f = 0;\
    if (gb->processor->reg & 1) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    gb->processor->reg = (gb->processor->reg >> 1) | top;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void sra_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    uint8_t top = val & 0x80;
    gb->processor->f = 0;
    if (val & 1) gb->processor->f |= PROCESSOR_CARRY_BIT;
    val = (val >> 1) | top;
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define SWAP_R(reg)\
static void swap_##reg(GameBoy * const gb) {\
    gb->processor->f = 0;\
    gb->processor->reg = ((gb->processor->reg >> 4) | (gb->processor->reg << 4));\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void swap_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    gb->processor->f = 0;
    val = ((val >> 4) | (val << 4));
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define SRL_R(reg)\
static void srl_##reg(GameBoy * const gb) {\
    gb->processor->f = 0;\
    if (gb->processor->reg & 1) gb->processor->f |= PROCESSOR_CARRY_BIT;\
    gb->processor->reg >>= 1;\
    if (gb->processor->reg == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

static void srl_dhl(GameBoy * const gb) {
    uint8_t val = gameboy_read(gb, gb->processor->hl);
    gameboy_cycle(gb);
    gb->processor->f = 0;
    if (val & 1) gb->processor->f |= PROCESSOR_CARRY_BIT;
    val >>= 1;
    gameboy_write(gb, gb->processor->hl, val);
    gameboy_cycle(gb);
    if (val == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;
}

#define BIT_N_R(num, reg)\
static void bit_##num##_##reg(GameBoy * const gb) {\
    gb->processor->f &= PROCESSOR_CARRY_BIT;\
    gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (((1 << num) & gb->processor->reg) == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
}

#define BIT_N_DHL(num)\
static void bit_##num##_dhl(GameBoy * const gb) {\
    gb->processor->f &= PROCESSOR_CARRY_BIT;\
    gb->processor->f |= PROCESSOR_HALF_BIT;\
    if (((1 << num) & gameboy_read(gb, gb->processor->hl)) == 0) gb->processor->f |= PROCESSOR_ZERO_BIT;\
    gameboy_cycle(gb);\
}

#define RES_N_R(num, reg)\
static void res_##num##_##reg(GameBoy * const gb) {\
    gb->processor->reg &= ~(1 << num);\
}

#define RES_N_DHL(num)\
static void res_##num##_dhl(GameBoy * const gb) {\
    uint8_t mask = gameboy_read(gb, gb->processor->hl) & ~(1 << num);\
    gameboy_cycle(gb);\
    gameboy_write(gb, gb->processor->hl, mask);\
    gameboy_cycle(gb);\
}

#define SET_N_R(num, reg)\
static void set_##num##_##reg(GameBoy * const gb) {\
    gb->processor->reg |= (1 << num);\
}

#define SET_N_DHL(num)\
static void set_##num##_dhl(GameBoy * const gb) {\
    uint8_t mask = gameboy_read(gb, gb->processor->hl) | (1 << num);\
    gameboy_cycle(gb);\
    gameboy_write(gb, gb->processor->hl, mask);\
    gameboy_cycle(gb);\
}

/*0*/         /*1*/         /*2*/         /*3*/         /*4*/         /*5*/         /*6*/        /*7*/
/*8*/         /*9*/         /*A*/         /*B*/         /*C*/         /*D*/         /*E*/        /*F*/
RLC_R(b)      RLC_R(c)      RLC_R(d)      RLC_R(e)      RLC_R(h)      RLC_R(l)      /*RLC_DHL*/  RLC_R(a)      /*0*/
RRC_R(b)      RRC_R(c)      RRC_R(d)      RRC_R(e)      RRC_R(h)      RRC_R(l)      /*RRC_DHL*/  RRC_R(a)
RL_R(b)       RL_R(c)       RL_R(d)       RL_R(e)       RL_R(h)       RL_R(l)       /*RL_DHL*/   RL_R(a)       /*1*/
RR_R(b)       RR_R(c)       RR_R(d)       RR_R(e)       RR_R(h)       RR_R(l)       /*RR_DHL*/   RR_R(a)
SLA_R(b)      SLA_R(c)      SLA_R(d)      SLA_R(e)      SLA_R(h)      SLA_R(l)      /*SLA_DHL*/  SLA_R(a)      /*2*/
SRA_R(b)      SRA_R(c)      SRA_R(d)      SRA_R(e)      SRA_R(h)      SRA_R(l)      /*SRA_DHL*/  SRA_R(a)
SWAP_R(b)     SWAP_R(c)     SWAP_R(d)     SWAP_R(e)     SWAP_R(h)     SWAP_R(l)     /*SWAP_DHL*/ SWAP_R(a)     /*3*/
SRL_R(b)      SRL_R(c)      SRL_R(d)      SRL_R(e)      SRL_R(h)      SRL_R(l)      /*SRL_DHL*/  SRL_R(a)
BIT_N_R(0, b) BIT_N_R(0, c) BIT_N_R(0, d) BIT_N_R(0, e) BIT_N_R(0, h) BIT_N_R(0, l) BIT_N_DHL(0) BIT_N_R(0, a) /*4*/
BIT_N_R(1, b) BIT_N_R(1, c) BIT_N_R(1, d) BIT_N_R(1, e) BIT_N_R(1, h) BIT_N_R(1, l) BIT_N_DHL(1) BIT_N_R(1, a)
BIT_N_R(2, b) BIT_N_R(2, c) BIT_N_R(2, d) BIT_N_R(2, e) BIT_N_R(2, h) BIT_N_R(2, l) BIT_N_DHL(2) BIT_N_R(2, a) /*5*/
BIT_N_R(3, b) BIT_N_R(3, c) BIT_N_R(3, d) BIT_N_R(3, e) BIT_N_R(3, h) BIT_N_R(3, l) BIT_N_DHL(3) BIT_N_R(3, a)
BIT_N_R(4, b) BIT_N_R(4, c) BIT_N_R(4, d) BIT_N_R(4, e) BIT_N_R(4, h) BIT_N_R(4, l) BIT_N_DHL(4) BIT_N_R(4, a) /*6*/
BIT_N_R(5, b) BIT_N_R(5, c) BIT_N_R(5, d) BIT_N_R(5, e) BIT_N_R(5, h) BIT_N_R(5, l) BIT_N_DHL(5) BIT_N_R(5, a)
BIT_N_R(6, b) BIT_N_R(6, c) BIT_N_R(6, d) BIT_N_R(6, e) BIT_N_R(6, h) BIT_N_R(6, l) BIT_N_DHL(6) BIT_N_R(6, a) /*7*/
BIT_N_R(7, b) BIT_N_R(7, c) BIT_N_R(7, d) BIT_N_R(7, e) BIT_N_R(7, h) BIT_N_R(7, l) BIT_N_DHL(7) BIT_N_R(7, a)
RES_N_R(0, b) RES_N_R(0, c) RES_N_R(0, d) RES_N_R(0, e) RES_N_R(0, h) RES_N_R(0, l) RES_N_DHL(0) RES_N_R(0, a) /*8*/
RES_N_R(1, b) RES_N_R(1, c) RES_N_R(1, d) RES_N_R(1, e) RES_N_R(1, h) RES_N_R(1, l) RES_N_DHL(1) RES_N_R(1, a)
RES_N_R(2, b) RES_N_R(2, c) RES_N_R(2, d) RES_N_R(2, e) RES_N_R(2, h) RES_N_R(2, l) RES_N_DHL(2) RES_N_R(2, a) /*9*/
RES_N_R(3, b) RES_N_R(3, c) RES_N_R(3, d) RES_N_R(3, e) RES_N_R(3, h) RES_N_R(3, l) RES_N_DHL(3) RES_N_R(3, a)
RES_N_R(4, b) RES_N_R(4, c) RES_N_R(4, d) RES_N_R(4, e) RES_N_R(4, h) RES_N_R(4, l) RES_N_DHL(4) RES_N_R(4, a) /*A*/
RES_N_R(5, b) RES_N_R(5, c) RES_N_R(5, d) RES_N_R(5, e) RES_N_R(5, h) RES_N_R(5, l) RES_N_DHL(5) RES_N_R(5, a)
RES_N_R(6, b) RES_N_R(6, c) RES_N_R(6, d) RES_N_R(6, e) RES_N_R(6, h) RES_N_R(6, l) RES_N_DHL(6) RES_N_R(6, a) /*B*/
RES_N_R(7, b) RES_N_R(7, c) RES_N_R(7, d) RES_N_R(7, e) RES_N_R(7, h) RES_N_R(7, l) RES_N_DHL(7) RES_N_R(7, a)
SET_N_R(0, b) SET_N_R(0, c) SET_N_R(0, d) SET_N_R(0, e) SET_N_R(0, h) SET_N_R(0, l) SET_N_DHL(0) SET_N_R(0, a) /*C*/
SET_N_R(1, b) SET_N_R(1, c) SET_N_R(1, d) SET_N_R(1, e) SET_N_R(1, h) SET_N_R(1, l) SET_N_DHL(1) SET_N_R(1, a)
SET_N_R(2, b) SET_N_R(2, c) SET_N_R(2, d) SET_N_R(2, e) SET_N_R(2, h) SET_N_R(2, l) SET_N_DHL(2) SET_N_R(2, a) /*D*/
SET_N_R(3, b) SET_N_R(3, c) SET_N_R(3, d) SET_N_R(3, e) SET_N_R(3, h) SET_N_R(3, l) SET_N_DHL(3) SET_N_R(3, a)
SET_N_R(4, b) SET_N_R(4, c) SET_N_R(4, d) SET_N_R(4, e) SET_N_R(4, h) SET_N_R(4, l) SET_N_DHL(4) SET_N_R(4, a) /*E*/
SET_N_R(5, b) SET_N_R(5, c) SET_N_R(5, d) SET_N_R(5, e) SET_N_R(5, h) SET_N_R(5, l) SET_N_DHL(5) SET_N_R(5, a)
SET_N_R(6, b) SET_N_R(6, c) SET_N_R(6, d) SET_N_R(6, e) SET_N_R(6, h) SET_N_R(6, l) SET_N_DHL(6) SET_N_R(6, a) /*F*/
SET_N_R(7, b) SET_N_R(7, c) SET_N_R(7, d) SET_N_R(7, e) SET_N_R(7, h) SET_N_R(7, l) SET_N_DHL(7) SET_N_R(7, a)

static void (*prefixed_instructions[])(GameBoy * const gb) = {
    /*0*/    /*1*/    /*2*/    /*3*/    /*4*/    /*5*/    /*6*/      /*7*/
    /*8*/    /*9*/    /*A*/    /*B*/    /*C*/    /*D*/    /*E*/      /*F*/
    rlc_b,   rlc_c,   rlc_d,   rlc_e,   rlc_h,   rlc_l,   rlc_dhl,   rlc_a,   /*0*/
    rrc_b,   rrc_c,   rrc_d,   rrc_e,   rrc_h,   rrc_l,   rrc_dhl,   rrc_a,
    rl_b,    rl_c,    rl_d,    rl_e,    rl_h,    rl_l,    rl_dhl,    rl_a,    /*1*/
    rr_b,    rr_c,    rr_d,    rr_e,    rr_h,    rr_l,    rr_dhl,    rr_a,
    sla_b,   sla_c,   sla_d,   sla_e,   sla_h,   sla_l,   sla_dhl,   sla_a,   /*2*/
    sra_b,   sra_c,   sra_d,   sra_e,   sra_h,   sra_l,   sra_dhl,   sra_a,
    swap_b,  swap_c,  swap_d,  swap_e,  swap_h,  swap_l,  swap_dhl,  swap_a,  /*3*/
    srl_b,   srl_c,   srl_d,   srl_e,   srl_h,   srl_l,   srl_dhl,   srl_a,
    bit_0_b, bit_0_c, bit_0_d, bit_0_e, bit_0_h, bit_0_l, bit_0_dhl, bit_0_a, /*4*/
    bit_1_b, bit_1_c, bit_1_d, bit_1_e, bit_1_h, bit_1_l, bit_1_dhl, bit_1_a,
    bit_2_b, bit_2_c, bit_2_d, bit_2_e, bit_2_h, bit_2_l, bit_2_dhl, bit_2_a, /*5*/
    bit_3_b, bit_3_c, bit_3_d, bit_3_e, bit_3_h, bit_3_l, bit_3_dhl, bit_3_a,
    bit_4_b, bit_4_c, bit_4_d, bit_4_e, bit_4_h, bit_4_l, bit_4_dhl, bit_4_a, /*6*/
    bit_5_b, bit_5_c, bit_5_d, bit_5_e, bit_5_h, bit_5_l, bit_5_dhl, bit_5_a,
    bit_6_b, bit_6_c, bit_6_d, bit_6_e, bit_6_h, bit_6_l, bit_6_dhl, bit_6_a, /*7*/
    bit_7_b, bit_7_c, bit_7_d, bit_7_e, bit_7_h, bit_7_l, bit_7_dhl, bit_7_a,
    res_0_b, res_0_c, res_0_d, res_0_e, res_0_h, res_0_l, res_0_dhl, res_0_a, /*8*/
    res_1_b, res_1_c, res_1_d, res_1_e, res_1_h, res_1_l, res_1_dhl, res_1_a,
    res_2_b, res_2_c, res_2_d, res_2_e, res_2_h, res_2_l, res_2_dhl, res_2_a, /*9*/
    res_3_b, res_3_c, res_3_d, res_3_e, res_3_h, res_3_l, res_3_dhl, res_3_a,
    res_4_b, res_4_c, res_4_d, res_4_e, res_4_h, res_4_l, res_4_dhl, res_4_a, /*A*/
    res_5_b, res_5_c, res_5_d, res_5_e, res_5_h, res_5_l, res_5_dhl, res_5_a,
    res_6_b, res_6_c, res_6_d, res_6_e, res_6_h, res_6_l, res_6_dhl, res_6_a, /*B*/
    res_7_b, res_7_c, res_7_d, res_7_e, res_7_h, res_7_l, res_7_dhl, res_7_a,
    set_0_b, set_0_c, set_0_d, set_0_e, set_0_h, set_0_l, set_0_dhl, set_0_a, /*C*/
    set_1_b, set_1_c, set_1_d, set_1_e, set_1_h, set_1_l, set_1_dhl, set_1_a,
    set_2_b, set_2_c, set_2_d, set_2_e, set_2_h, set_2_l, set_2_dhl, set_2_a, /*D*/
    set_3_b, set_3_c, set_3_d, set_3_e, set_3_h, set_3_l, set_3_dhl, set_3_a,
    set_4_b, set_4_c, set_4_d, set_4_e, set_4_h, set_4_l, set_4_dhl, set_4_a, /*E*/
    set_5_b, set_5_c, set_5_d, set_5_e, set_5_h, set_5_l, set_5_dhl, set_5_a,
    set_6_b, set_6_c, set_6_d, set_6_e, set_6_h, set_6_l, set_6_dhl, set_6_a, /*F*/
    set_7_b, set_7_c, set_7_d, set_7_e, set_7_h, set_7_l, set_7_dhl, set_7_a
};

static void prefix_cb(GameBoy * const gb) {
    uint8_t opcode = gameboy_read(gb, gb->processor->pc++);
    gameboy_cycle(gb);
    prefixed_instructions[opcode](gb);
}

// Helper functions
RST_NNH(40)
RST_NNH(48)
RST_NNH(50)
RST_NNH(58)
RST_NNH(60)

/*0*/          /*1*/         /*2*/         /*3*/         /*4*/           /*5*/        /*6*/         /*7*/
/*8*/          /*9*/         /*A*/         /*B*/         /*C*/           /*D*/        /*E*/         /*F*/
/*NOP*/        LD_RR_D16(bc) LD_DRR_A(bc)  INC_RR(bc)    INC_R(b)        DEC_R(b)     LD_R_D8(b)    /*RLCA*/     /*0*/
/*LD_DA16_SP*/ ADD_HL_RR(bc) LD_A_DRR(bc)  DEC_RR(bc)    INC_R(c)        DEC_R(c)     LD_R_D8(c)    /*RRCA*/
/*STOP*/       LD_RR_D16(de) LD_DRR_A(de)  INC_RR(de)    INC_R(d)        DEC_R(d)     LD_R_D8(d)    /*RLA*/      /*1*/
/*JR_R8*/      ADD_HL_RR(de) LD_A_DRR(de)  DEC_RR(de)    INC_R(e)        DEC_R(e)     LD_R_D8(e)    /*RRA*/
/*JR_NZ_R8*/   LD_RR_D16(hl) /*LD_DHLI_A*/ INC_RR(hl)    INC_R(h)        DEC_R(h)     LD_R_D8(h)    /*DAA*/      /*2*/
/*JR_Z_R8*/    ADD_HL_RR(hl) /*LD_A_DHLI*/ DEC_RR(hl)    INC_R(l)        DEC_R(l)     LD_R_D8(l)    /*CPL*/
/*JR_NC_R8*/   LD_RR_D16(sp) /*LD_DHLD_A*/ INC_RR(sp)    /*INC_DHL*/     /*DEC_DHL*/  /*LD_DHL_D8*/ /*SCF*/      /*3*/
/*JR_C_R8*/    ADD_HL_RR(sp) /*LD_A_DHLD*/ DEC_RR(sp)    INC_R(a)        DEC_R(a)     LD_R_D8(a)    /*CCF*/
LD_R_R(b, b)   LD_R_R(b, c)  LD_R_R(b, d)  LD_R_R(b, e)  LD_R_R(b, h)    LD_R_R(b, l) LD_R_DHL(b)   LD_R_R(b, a) /*4*/
LD_R_R(c, b)   LD_R_R(c, c)  LD_R_R(c, d)  LD_R_R(c, e)  LD_R_R(c, h)    LD_R_R(c, l) LD_R_DHL(c)   LD_R_R(c, a)
LD_R_R(d, b)   LD_R_R(d, c)  LD_R_R(d, d)  LD_R_R(d, e)  LD_R_R(d, h)    LD_R_R(d, l) LD_R_DHL(d)   LD_R_R(d, a) /*5*/
LD_R_R(e, b)   LD_R_R(e, c)  LD_R_R(e, d)  LD_R_R(e, e)  LD_R_R(e, h)    LD_R_R(e, l) LD_R_DHL(e)   LD_R_R(e, a)
LD_R_R(h, b)   LD_R_R(h, c)  LD_R_R(h, d)  LD_R_R(h, e)  LD_R_R(h, h)    LD_R_R(h, l) LD_R_DHL(h)   LD_R_R(h, a) /*6*/
LD_R_R(l, b)   LD_R_R(l, c)  LD_R_R(l, d)  LD_R_R(l, e)  LD_R_R(l, h)    LD_R_R(l, l) LD_R_DHL(l)   LD_R_R(l, a)
LD_DHL_R(b)    LD_DHL_R(c)   LD_DHL_R(d)   LD_DHL_R(e)   LD_DHL_R(h)     LD_DHL_R(l)  /*HALT*/      LD_DHL_R(a)  /*7*/
LD_R_R(a, b)   LD_R_R(a, c)  LD_R_R(a, d)  LD_R_R(a, e)  LD_R_R(a, h)    LD_R_R(a, l) LD_R_DHL(a)   LD_R_R(a, a)
ADD_A_R(b)     ADD_A_R(c)    ADD_A_R(d)    ADD_A_R(e)    ADD_A_R(h)      ADD_A_R(l)   /*ADD_A_DHL*/ ADD_A_R(a)   /*8*/
ADC_A_R(b)     ADC_A_R(c)    ADC_A_R(d)    ADC_A_R(e)    ADC_A_R(h)      ADC_A_R(l)   /*ADC_A_DHL*/ ADC_A_R(a)
SUB_A_R(b)     SUB_A_R(c)    SUB_A_R(d)    SUB_A_R(e)    SUB_A_R(h)      SUB_A_R(l)   /*SUB_A_DHL*/ SUB_A_R(a)   /*9*/
SBC_A_R(b)     SBC_A_R(c)    SBC_A_R(d)    SBC_A_R(e)    SBC_A_R(h)      SBC_A_R(l)   /*SBC_A_DHL*/ SBC_A_R(a)
AND_A_R(b)     AND_A_R(c)    AND_A_R(d)    AND_A_R(e)    AND_A_R(h)      AND_A_R(l)   /*AND_A_DHL*/ AND_A_R(a)   /*A*/
XOR_A_R(b)     XOR_A_R(c)    XOR_A_R(d)    XOR_A_R(e)    XOR_A_R(h)      XOR_A_R(l)   /*XOR_A_DHL*/ XOR_A_R(a)
OR_A_R(b)      OR_A_R(c)     OR_A_R(d)     OR_A_R(e)     OR_A_R(h)       OR_A_R(l)    /*OR_A_DHL*/  OR_A_R(a)    /*B*/
CP_A_R(b)      CP_A_R(c)     CP_A_R(d)     CP_A_R(e)     CP_A_R(h)       CP_A_R(l)    /*CP_A_DHL*/  CP_A_R(a)
/*RET_NZ*/     POP_RR(bc)    /*JP_NZ_A16*/ /*JP_A16*/    /*CALL_NZ_A16*/ PUSH_RR(bc)  /*ADD_A_D8*/  RST_NNH(00)  /*C*/
/*RET_Z*/      /*RET*/       /*JP_Z_A16*/  /*PREFIX_CB*/ /*CALL_Z_A16*/  /*CALL_A16*/ /*ADC_A_D8*/  RST_NNH(08)
/*RET_NC*/     POP_RR(de)    /*JP_NC_A16*/ /*INVOP*/     /*CALL_NC_A16*/ PUSH_RR(de)  /*SUB_A_D8*/  RST_NNH(10)  /*D*/
/*RET_C*/      /*RETI*/      /*JP_C_A16*/  /*INVOP*/     /*CALL_C_A16*/  /*INVOP*/    /*SBC_A_D8*/  RST_NNH(18)
/*LDH_DA8_A*/  POP_RR(hl)    /*LD_DC_A*/   /*INVOP*/     /*INVOP*/       PUSH_RR(hl)  /*AND_A_D8*/  RST_NNH(20)  /*E*/
/*ADD_SP_R8*/  /*JP_DHL*/    /*LD_DA16_A*/ /*INVOP*/     /*INVOP*/       /*INVOP*/    /*XOR_A_D8*/  RST_NNH(28)
/*LDH_A_DA8*/  /*POP_AF*/    /*LD_A_DC*/   /*DI*/        /*INVOP*/       PUSH_RR(af)  /*OR_A_D8*/   RST_NNH(30)  /*F*/
/*LD_HL_SPR8*/ /*LD_SP_HL*/  /*LD_A_DA16*/ /*EI*/        /*INVOP*/       /*INVOP*/    /*CP_A_D8*/   RST_NNH(38)

static void (*instructions[])(GameBoy * const gb) = {
    /*0*/        /*1*/      /*2*/          /*3*/      /*4*/        /*5*/     /*6*/      /*7*/
    /*8*/        /*9*/      /*A*/          /*B*/      /*C*/        /*D*/     /*E*/      /*F*/
    nop,         ld_bc_d16, ld_dbc_a,      inc_bc,    inc_b,       dec_b,    ld_b_d8,   rlca,     /*0*/
    ld_da16_sp,  add_hl_bc, ld_a_dbc,      dec_bc,    inc_c,       dec_c,    ld_c_d8,   rrca,
    stop,        ld_de_d16, ld_dde_a,      inc_de,    inc_d,       dec_d,    ld_d_d8,   rla,      /*1*/
    jr_r8,       add_hl_de, ld_a_dde,      dec_de,    inc_e,       dec_e,    ld_e_d8,   rra,
    jr_nz_r8,    ld_hl_d16, ld_dhli_a,     inc_hl,    inc_h,       dec_h,    ld_h_d8,   daa,      /*2*/
    jr_z_r8,     add_hl_hl, ld_a_dhli,     dec_hl,    inc_l,       dec_l,    ld_l_d8,   cpl,
    jr_nc_r8,    ld_sp_d16, ld_dhld_a,     inc_sp,    inc_dhl,     dec_dhl,  ld_dhl_d8, scf,      /*3*/
    jr_c_r8,     add_hl_sp, ld_a_dhld,     dec_sp,    inc_a,       dec_a,    ld_a_d8,   ccf,
    ld_b_b,      ld_b_c,    ld_b_d,        ld_b_e,    ld_b_h,      ld_b_l,   ld_b_dhl,  ld_b_a,   /*4*/
    ld_c_b,      ld_c_c,    ld_c_d,        ld_c_e,    ld_c_h,      ld_c_l,   ld_c_dhl,  ld_c_a,
    ld_d_b,      ld_d_c,    ld_d_d,        ld_d_e,    ld_d_h,      ld_d_l,   ld_d_dhl,  ld_d_a,   /*5*/
    ld_e_b,      ld_e_c,    ld_e_d,        ld_e_e,    ld_e_h,      ld_e_l,   ld_e_dhl,  ld_e_a,
    ld_h_b,      ld_h_c,    ld_h_d,        ld_h_e,    ld_h_h,      ld_h_l,   ld_h_dhl,  ld_h_a,   /*6*/
    ld_l_b,      ld_l_c,    ld_l_d,        ld_l_e,    ld_l_h,      ld_l_l,   ld_l_dhl,  ld_l_a,
    ld_dhl_b,    ld_dhl_c,  ld_dhl_d,      ld_dhl_e,  ld_dhl_h,    ld_dhl_l, halt,      ld_dhl_a, /*7*/
    ld_a_b,      ld_a_c,    ld_a_d,        ld_a_e,    ld_a_h,      ld_a_l,   ld_a_dhl,  ld_a_a,
    add_a_b,     add_a_c,   add_a_d,       add_a_e,   add_a_h,     add_a_l,  add_a_dhl, add_a_a,  /*8*/
    adc_a_b,     adc_a_c,   adc_a_d,       adc_a_e,   adc_a_h,     adc_a_l,  adc_a_dhl, adc_a_a,
    sub_a_b,     sub_a_c,   sub_a_d,       sub_a_e,   sub_a_h,     sub_a_l,  sub_a_dhl, sub_a_a,  /*9*/
    sbc_a_b,     sbc_a_c,   sbc_a_d,       sbc_a_e,   sbc_a_h,     sbc_a_l,  sbc_a_dhl, sbc_a_a,
    and_a_b,     and_a_c,   and_a_d,       and_a_e,   and_a_h,     and_a_l,  and_a_dhl, and_a_a,  /*A*/
    xor_a_b,     xor_a_c,   xor_a_d,       xor_a_e,   xor_a_h,     xor_a_l,  xor_a_dhl, xor_a_a,
    or_a_b,      or_a_c,    or_a_d,        or_a_e,    or_a_h,      or_a_l,   or_a_dhl,  or_a_a,   /*B*/
    cp_a_b,      cp_a_c,    cp_a_d,        cp_a_e,    cp_a_h,      cp_a_l,   cp_a_dhl,  cp_a_a,
    ret_nz,      pop_bc,    jp_nz_a16,     jp_a16,    call_nz_a16, push_bc,  add_a_d8,  rst_00h,  /*C*/
    ret_z,       ret,       jp_z_a16,      prefix_cb, call_z_a16,  call_a16, adc_a_d8,  rst_08h,
    ret_nc,      pop_de,    jp_nc_a16,     inv_op,    call_nc_a16, push_de,  sub_a_d8,  rst_10h,  /*D*/
    ret_c,       reti,      jp_c_a16,      inv_op,    call_c_a16,  inv_op,   sbc_a_d8,  rst_18h,
    ldh_da8_a,   pop_hl,    ldh_dc_a,      inv_op,    inv_op,      push_hl,  and_a_d8,  rst_20h,  /*E*/
    add_sp_r8,   jp_hl,     ld_da16_a,     inv_op,    inv_op,      inv_op,   xor_a_d8,  rst_28h,
    ldh_a_da8,   pop_af,    ldh_a_dc,      di,        inv_op,      push_af,  or_a_d8,   rst_30h,  /*F*/
    ld_hl_sp_r8, ld_sp_hl,  ld_a_da16,     ei,        inv_op,      inv_op,   cp_a_d8,   rst_38h
};

void processor_process_instruction(GameBoy * const gb) {
    if (gb->processor->halt_mode) {
        uint8_t interrupts = gb->interrupt_controller->flags & gb->interrupt_controller->enables & 0x1F;
        if (interrupts == 0) {
            gameboy_cycle(gb);
            return; // Notice this and don't reorder it
        }
        gb->processor->halt_mode = false;
    }

    if (gb->interrupt_controller->ime && !gb->processor->skip_next_interrupt) {
        uint8_t interrupts = gb->interrupt_controller->flags & gb->interrupt_controller->enables;
        if (interrupts & VBLANK_INTERRUPT_BIT) {
            gb->interrupt_controller->ime = false;
            gameboy_write(gb, INTERRUPT_FLAGS_ADDRESS, gb->interrupt_controller->flags & ~VBLANK_INTERRUPT_BIT);
            gameboy_cycle(gb);
            rst_40h(gb);
            gameboy_cycle(gb);
        }
        else if (interrupts & LCD_STAT_INTERRUPT_BIT) {
            gb->interrupt_controller->ime = false;
            gameboy_write(gb, INTERRUPT_FLAGS_ADDRESS, gb->interrupt_controller->flags & ~LCD_STAT_INTERRUPT_BIT);
            gameboy_cycle(gb);
            rst_48h(gb);
            gameboy_cycle(gb);
        }
        else if (interrupts & TIMER_INTERRUPT_BIT) {
            gb->interrupt_controller->ime = false;
            gameboy_write(gb, INTERRUPT_FLAGS_ADDRESS, gb->interrupt_controller->flags & ~TIMER_INTERRUPT_BIT);
            gameboy_cycle(gb);
            rst_50h(gb);
            gameboy_cycle(gb);
        }
        else if (interrupts & SERIAL_INTERRUPT_BIT) {
            gb->interrupt_controller->ime = false;
            gameboy_write(gb, INTERRUPT_FLAGS_ADDRESS, gb->interrupt_controller->flags & ~SERIAL_INTERRUPT_BIT);
            gameboy_cycle(gb);
            rst_58h(gb);
            gameboy_cycle(gb);
        }
        else if (interrupts & JOYPAD_INTERRUPT_BIT) {
            gb->interrupt_controller->ime = false;
            gameboy_write(gb, INTERRUPT_FLAGS_ADDRESS, gb->interrupt_controller->flags & ~JOYPAD_INTERRUPT_BIT);
            gameboy_cycle(gb);
            rst_60h(gb);
            gameboy_cycle(gb);
        }
    }
    gb->processor->skip_next_interrupt = false;

    if (gb->interrupt_controller->cycles_until_ime > 0) gb->interrupt_controller->cycles_until_ime -= 1;
    if (gb->interrupt_controller->cycles_until_ime == 0) {
        gb->interrupt_controller->cycles_until_ime = -1;
        gb->interrupt_controller->ime = 1;
    }

    uint8_t opcode = gameboy_read(gb, gb->processor->pc);
    if (gb->processor->skip_pc_increment) gb->processor->skip_pc_increment = false;
    else gb->processor->pc += 1;
    gameboy_cycle(gb);

    instructions[opcode](gb);
}
