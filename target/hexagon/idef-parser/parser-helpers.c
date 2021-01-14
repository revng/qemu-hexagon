/*
 * Copyright(c) 2019-2020 rev.ng Srls. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; withOUT even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "idef-parser.h"
#include "parser-helpers.h"
#include "idef-parser.tab.h"
#include "idef-parser.yy.h"

const char *COND_EQ = "TCG_COND_EQ";
const char *COND_NE = "TCG_COND_NE";
const char *COND_GT = "TCG_COND_GT";
const char *COND_LT = "TCG_COND_LT";
const char *COND_GE = "TCG_COND_GE";
const char *COND_LE = "TCG_COND_LE";
const char *COND_GTU = "TCG_COND_GTU";
const char *COND_LTU = "TCG_COND_LTU";
const char *COND_GEU = "TCG_COND_GEU";
const char *COND_LEU = "TCG_COND_LEU";

const char *creg_str[] = {"HEX_REG_SP", "HEX_REG_FP", "HEX_REG_LR",
                          "HEX_REG_GP", "HEX_REG_LC0", "HEX_REG_LC1",
                          "HEX_REG_SA0", "HEX_REG_SA1"};

void yyerror(YYLTYPE *locp,
             yyscan_t scanner __attribute__((unused)),
             context_t *c,
             const char *s)
{
    const char *code_ptr = c->input_buffer;

    fprintf(stderr, "WARNING (%s): '%s'\n", c->inst.name, s);

    fprintf(stderr, "Problematic range: ");
    for (int i = locp->first_column; i < locp->last_column; i++) {
        if (code_ptr[i] != '\n') {
            fprintf(stderr, "%c", code_ptr[i]);
        }
    }
    fprintf(stderr, "\n");

    for (int i = 0;
         i < 80 &&
         code_ptr[locp->first_column - 10 + i] != '\0' &&
         code_ptr[locp->first_column - 10 + i] != '\n';
         i++) {
        fprintf(stderr, "%c", code_ptr[locp->first_column - 10 + i]);
    }
    fprintf(stderr, "\n");
    for (int i = 0; i < 9; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^");
    for (int i = 0; i < (locp->last_column - locp->first_column) - 1; i++) {
        fprintf(stderr, "~");
    }
    fprintf(stderr, "\n");
    c->inst.error_count++;
}

bool is_direct_predicate(t_hex_value *value)
{
    return value->pre.id >= '0' && value->pre.id <= '3';
}

/* Print functions */
void str_print(context_t *c, YYLTYPE *locp, char *string)
{
    EMIT(c, "%s", string);
}


void uint64_print(context_t *c, YYLTYPE *locp, uint64_t *num)
{
    EMIT(c, "%" PRIu64, *num);
}

void int_print(context_t *c, YYLTYPE *locp, int *num)
{
    EMIT(c, "%d", *num);
}

void tmp_print(context_t *c, YYLTYPE *locp, t_hex_tmp *tmp)
{
    EMIT(c, "tmp_");
    EMIT(c, "%d", tmp->index);
}

void pre_print(context_t *c, YYLTYPE *locp, t_hex_pre *pre, bool is_dotnew)
{
    char suffix = is_dotnew ? 'N' : 'V';
    EMIT(c, "P%c%c", pre->id, suffix);
}

void reg_compose(context_t *c, YYLTYPE *locp, t_hex_reg *reg, char reg_id[5])
{
    switch (reg->type) {
    case GENERAL_PURPOSE:
        reg_id[0] = 'R';
        break;
    case CONTROL:
        reg_id[0] = 'C';
        break;
    case SYSTEM:
        reg_id[0] = 'S';
        break;
    case MODIFIER:
        reg_id[0] = 'M';
        break;
    case DOTNEW:
        /* The DOTNEW case is managed by the upper level function */
        break;
    }
    switch (reg->bit_width) {
    case 32:
        reg_id[1] = reg->id;
        reg_id[2] = 'V';
        break;
    case 64:
        reg_id[1] = reg->id;
        reg_id[2] = reg->id;
        reg_id[3] = 'V';
        break;
    default:
        yyassert(c, locp, false, "Unhandled register bit width!\n");
    }
}

void reg_print(context_t *c, YYLTYPE *locp, t_hex_reg *reg)
{
  if (reg->type == DOTNEW) {
    EMIT(c, "N%cN", reg->id);
  } else {
    char reg_id[5] = { 0 };
    reg_compose(c, locp, reg, reg_id);
    EMIT(c, "%s", reg_id);
  }
}

void imm_print(context_t *c, YYLTYPE *locp, t_hex_imm *imm)
{
    switch (imm->type) {
    case I:
        EMIT(c, "i");
        break;
    case VARIABLE:
        EMIT(c, "%ciV", imm->id);
        break;
    case VALUE:
        EMIT(c, "%" PRIu64, imm->value);
        break;
    case QEMU_TMP:
        EMIT(c, "qemu_tmp_%" PRIu64, imm->index);
        break;
    case IMM_PC:
        EMIT(c, "dc->pc");
        break;
    case IMM_CONSTEXT:
        EMIT(c, "insn->extension_valid");
        break;
    default:
        yyassert(c, locp, false, "Cannot print this expression!");
    }
}

void var_print(context_t *c, YYLTYPE *locp, t_hex_var *var)
{
    EMIT(c, "%s", var->name);
}

void rvalue_out(context_t *c, YYLTYPE *locp, void *pointer)
{
  t_hex_value *rvalue = (t_hex_value *) pointer;
  switch (rvalue->type) {
  case REGISTER:
      reg_print(c, locp, &rvalue->reg);
      break;
  case TEMP:
      tmp_print(c, locp, &rvalue->tmp);
      break;
  case IMMEDIATE:
      imm_print(c, locp, &rvalue->imm);
      break;
  case VARID:
      var_print(c, locp, &rvalue->var);
      break;
  case PREDICATE:
      pre_print(c, locp, &rvalue->pre, rvalue->is_dotnew);
      break;
  default:
      yyassert(c, locp, false, "Cannot print this expression!");
  }
}

/* Copy output code buffer */
void commit(context_t *c)
{
    /* Emit instruction pseudocode */
    EMIT_SIG(c, "\n" START_COMMENT " ");
    for (char *x = c->inst.code_begin; x < c->inst.code_end; x++) {
        EMIT_SIG(c, "%c", *x);
    }
    EMIT_SIG(c, " " END_COMMENT "\n");

    /* Commit instruction code to output file */
    fwrite(c->signature_buffer, sizeof(char), c->signature_c, c->output_file);
    fwrite(c->out_buffer, sizeof(char), c->out_c, c->output_file);

    fwrite(c->signature_buffer, sizeof(char), c->signature_c, c->defines_file);
    fprintf(c->defines_file, ";\n");
}

const char *cmp_swap(context_t *c, YYLTYPE *locp, const char *type)
{
    if (type == COND_EQ) {
        return COND_EQ;
    } else if (type == COND_NE) {
        return COND_NE;
    } else if (type == COND_GT) {
        return COND_LT;
    } else if (type == COND_LT) {
        return COND_GT;
    } else if (type == COND_GE) {
        return COND_LE;
    } else if (type == COND_LE) {
        return COND_GE;
    } else if (type == COND_GTU) {
        return COND_LTU;
    } else if (type == COND_LTU) {
        return COND_GTU;
    } else if (type == COND_GEU) {
        return COND_LEU;
    } else if (type == COND_LEU) {
        return COND_GEU;
    } else {
        yyassert(c, locp, false, "Unhandled comparison swap!");
        return NULL;
    }
}

/* Temporary values creation */
t_hex_value gen_tmp(context_t *c, YYLTYPE *locp, int bit_width)
{
    t_hex_value rvalue;
    rvalue.type = TEMP;
    bit_width = (bit_width == 64) ? 64 : 32;
    rvalue.bit_width = bit_width;
    rvalue.is_unsigned = false;
    rvalue.is_dotnew = false;
    rvalue.is_symbol = false;
    rvalue.tmp.index = c->inst.tmp_count;
    OUT(c, locp, "TCGv_i", &bit_width, " tmp_", &c->inst.tmp_count,
        " = tcg_temp_new_i", &bit_width, "();\n");
    c->inst.tmp_count++;
    return rvalue;
}

t_hex_value gen_local_tmp(context_t *c, YYLTYPE *locp, int bit_width)
{
    t_hex_value rvalue;
    rvalue.type = TEMP;
    bit_width = (bit_width == 64) ? 64 : 32;
    rvalue.bit_width = bit_width;
    rvalue.is_unsigned = false;
    rvalue.is_dotnew = false;
    rvalue.is_symbol = false;
    rvalue.tmp.index = c->inst.tmp_count;
    OUT(c, locp, "TCGv_i", &bit_width, " tmp_", &c->inst.tmp_count,
        " = tcg_temp_local_new_i", &bit_width, "();\n");
    c->inst.tmp_count++;
    return rvalue;
}

void gen_dbg_str(context_t *c,
                 YYLTYPE *locp,
                 const char *str,
                 t_hex_value *val) {
    rvalue_materialize(c, locp, val);
    int width = val->bit_width;
    OUT(c, locp, "TCGv_ptr tmp_", &c->inst.tmp_count,
        " = tcg_const_local_ptr(", str, ");\n");
    OUT(c, locp, "gen_helper_dbg_str_i", &width, "(",
        "tmp_", &c->inst.tmp_count, ", ", val, ");\n");
    c->inst.tmp_count++;
}

t_hex_value gen_tmp_value(context_t *c,
                          YYLTYPE *locp,
                          const char *value,
                          int bit_width)
{
    t_hex_value rvalue;
    rvalue.type = TEMP;
    rvalue.bit_width = bit_width;
    rvalue.is_unsigned = false;
    rvalue.is_dotnew = false;
    rvalue.is_symbol = false;
    rvalue.tmp.index = c->inst.tmp_count;
    OUT(c, locp, "TCGv_i", &bit_width, " tmp_", &c->inst.tmp_count,
        " = tcg_const_i", &bit_width, "(", value, ");\n");
    c->inst.tmp_count++;
    return rvalue;
}

t_hex_value gen_imm_value(context_t *c __attribute__((unused)),
                          YYLTYPE *locp,
                          int value,
                          int bit_width)
{
    t_hex_value rvalue;
    rvalue.type = IMMEDIATE;
    rvalue.bit_width = bit_width;
    rvalue.is_unsigned = false;
    rvalue.is_dotnew = false;
    rvalue.is_symbol = false;
    rvalue.imm.type = VALUE;
    rvalue.imm.value = value;
    return rvalue;
}

void rvalue_free(context_t *c, YYLTYPE *locp, t_hex_value *rvalue)
{
    if (rvalue->type == TEMP) {
        const char *bit_suffix = (rvalue->bit_width == 64) ? "i64" : "i32";
        OUT(c, locp, "tcg_temp_free_", bit_suffix, "(", rvalue, ");\n");
    }
}

void rvalue_materialize(context_t *c, YYLTYPE *locp, t_hex_value *rvalue)
{
    if (rvalue->type == IMMEDIATE) {
        t_hex_value tmp = gen_tmp(c, locp, rvalue->bit_width);
        const char *bit_suffix = (rvalue->bit_width == 64) ? "i64" : "i32";
        OUT(c, locp, "tcg_gen_movi_", bit_suffix,
            "(", &tmp, ", ", rvalue, ");\n");
        tmp.is_symbol = rvalue->is_symbol;
        rvalue_free(c, locp, rvalue);
        *rvalue = tmp;
    }
}

void rvalue_extend(context_t *c, YYLTYPE *locp, t_hex_value *rvalue)
{
    if (rvalue->type == IMMEDIATE) {
        rvalue->bit_width = 64;
    } else {
        if (rvalue->bit_width == 32) {
            t_hex_value tmp = gen_tmp(c, locp, 64);
            const char *sign_suffix = (rvalue->is_unsigned) ? "u" : "";
            OUT(c, locp, "tcg_gen_ext", sign_suffix,
                "_i32_i64(", &tmp, ", ", rvalue, ");\n");
            rvalue_free(c, locp, rvalue);
            *rvalue = tmp;
        }
    }
}

void rvalue_truncate(context_t *c, YYLTYPE *locp, t_hex_value *rvalue)
{
    if (rvalue->type == IMMEDIATE) {
        rvalue->bit_width = 32;
    } else {
        if (rvalue->bit_width == 64) {
            t_hex_value tmp = gen_tmp(c, locp, 32);
            OUT(c, locp, "tcg_gen_trunc_i64_tl(", &tmp, ", ", rvalue, ");\n");
            rvalue_free(c, locp, rvalue);
            *rvalue = tmp;
        }
    }
}

int find_variable(context_t *c, YYLTYPE *locp, t_hex_value *varid) {
    for (int i = 0; i < c->inst.allocated_count; i++) {
        if (!strcmp(varid->var.name, c->inst.allocated[i].name)) {
            return i;
        }
    }
    return -1;
}

void varid_allocate(context_t *c,
                    YYLTYPE *locp,
                    t_hex_value *varid,
                    int width,
                    bool is_unsigned)
{
    varid->bit_width = width;
    const char *bit_suffix = width == 64 ? "64" : "32";
    yyassert(c, locp, c->inst.allocated_count < ALLOC_LIST_LEN,
             "Too many automatic variables required!");
    int index = find_variable(c, locp, varid);
    bool found = index != -1;
    if (found) {
        free((char *) varid->var.name);
        varid->var.name = c->inst.allocated[index].name;
        varid->bit_width = c->inst.allocated[index].bit_width;
        varid->is_unsigned = c->inst.allocated[index].is_unsigned;
    } else {
        OUT(c, locp, "TCGv_i", bit_suffix, " ", varid);
        OUT(c, locp, " = tcg_temp_local_new_i", bit_suffix, "();\n");
        c->inst.allocated[c->inst.allocated_count].name = varid->var.name;
        c->inst.allocated[c->inst.allocated_count].bit_width = width;
        c->inst.allocated[c->inst.allocated_count].is_unsigned = is_unsigned;
        c->inst.allocated_count++;
    }
}

void ea_free(context_t *c, YYLTYPE *locp)
{
    OUT(c, locp, "tcg_temp_free(EA);\n");
}

static inline t_hex_value gen_bin_cmp_ext(context_t *c,
                                          YYLTYPE *locp,
                                          const char *type,
                                          t_hex_value *op1,
                                          t_hex_value *op2,
                                          bool autofree)
{
#define IMM_IMM 0
#define IMM_REG 1
#define REG_IMM 2
#define REG_REG 3

    int op_types = (op1->type != IMMEDIATE) << 1 | (op2->type != IMMEDIATE);

    /* Find bit width of the two operands, if at least one is 64 bit use a */
    /* 64bit operation, eventually extend 32bit operands. */
    bool op_is64bit = op1->bit_width == 64 || op2->bit_width == 64;
    const char *bit_suffix = op_is64bit ? "i64" : "i32";
    int bit_width = (op_is64bit) ? 64 : 32;
    if (op_is64bit) {
        switch (op_types) {
        case IMM_REG:
            rvalue_extend(c, locp, op2);
            break;
        case REG_IMM:
            rvalue_extend(c, locp, op1);
            break;
        case REG_REG:
            rvalue_extend(c, locp, op1);
            rvalue_extend(c, locp, op2);
            break;
        }
    }

    t_hex_value res = gen_tmp(c, locp, bit_width);

    switch (op_types) {
    case IMM_IMM:
    {
        OUT(c, locp, "tcg_gen_movi_", bit_suffix,
            "(", &res, ", ", op1, " == ", op2, ");\n");
        break;
    }
    case IMM_REG:
    {
        t_hex_value swp = *op2;
        *op2 = *op1;
        *op1 = swp;
        /* Swap comparison direction */
        type = cmp_swap(c, locp, type);
        /* fallthrough */
    }
    case REG_IMM:
    {
        OUT(c, locp, "tcg_gen_setcondi_", bit_suffix, "(");
        OUT(c, locp, type, ", ", &res, ", ", op1, ", ", op2, ");\n");
        break;
    }
    case REG_REG:
    {
        OUT(c, locp, "tcg_gen_setcond_", bit_suffix, "(");
        OUT(c, locp, type, ", ", &res, ", ", op1, ", ", op2, ");\n");
        break;
    }
    default:
    {
        fprintf(stderr, "Error in evalutating immediateness!");
        abort();
    }
    }

    if (autofree) {
        /* Free operands */
        rvalue_free(c, locp, op1);
        rvalue_free(c, locp, op2);
    }

    return res;

#undef IMM_IMM
#undef IMM_REG
#undef REG_IMM
#undef REG_REG
}

t_hex_value gen_bin_cmp(context_t *c,
                        YYLTYPE *locp,
                        const char *type,
                        t_hex_value *op1,
                        t_hex_value *op2) {
    return gen_bin_cmp_ext(c, locp, type, op1, op2, true);
}

/* Code generation functions */
t_hex_value gen_bin_op(context_t *c,
                       YYLTYPE *locp,
                       enum op_type type,
                       t_hex_value *operand1,
                       t_hex_value *operand2)
{
#define IMM_IMM 0
#define IMM_REG 1
#define REG_IMM 2
#define REG_REG 3

    /* Replicate operands to avoid side effects */
    t_hex_value op1 = *operand1;
    t_hex_value op2 = *operand2;

    /* Enforce variables' size */
    if (op1.type == VARID) {
        int index = find_variable(c, locp, &op1);
        yyassert(c, locp, index >= 0, "Variable in bin_op must exist!\n");
        op1.bit_width = c->inst.allocated[index].bit_width;
    }
    if (op2.type == VARID) {
        int index = find_variable(c, locp, &op2);
        yyassert(c, locp, index >= 0, "Variable in bin_op must exist!\n");
        op2.bit_width = c->inst.allocated[index].bit_width;
    }

    int op_types = (op1.type != IMMEDIATE) << 1 | (op2.type != IMMEDIATE);

    /* Find bit width of the two operands, if at least one is 64 bit use a */
    /* 64bit operation, eventually extend 32bit operands. */
    bool op_is64bit = op1.bit_width == 64 || op2.bit_width == 64;
    /* Shift greater than 32 are 64 bits wide */
    if (type == ASL_OP && op2.type == IMMEDIATE &&
        op2.imm.type == VALUE && op2.imm.value >= 32)
        op_is64bit = true;
    const char *bit_suffix = op_is64bit ? "i64" : "i32";
    int bit_width = (op_is64bit) ? 64 : 32;
    /* Handle bit width */
    if (op_is64bit) {
        switch (op_types) {
        case IMM_REG:
            rvalue_extend(c, locp, &op2);
            break;
        case REG_IMM:
            rvalue_extend(c, locp, &op1);
            break;
        case REG_REG:
            rvalue_extend(c, locp, &op1);
            rvalue_extend(c, locp, &op2);
            break;
        }
    }
    t_hex_value res;
    if (op_types != IMM_IMM) {
        res = gen_tmp(c, locp, bit_width);
        res.type = TEMP;
    } else {
        res.type = IMMEDIATE;
        res.is_dotnew = false;
        res.is_symbol = false;
        res.imm.type = QEMU_TMP;
        res.imm.index = c->inst.qemu_tmp_count;
    }
    /* Handle signedness, if both unsigned -> result is unsigned, else signed */
    res.is_unsigned = op1.is_unsigned && op2.is_unsigned;

    switch (type) {
    case ADD_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ", &res,
                " = ", &op1, " + ", &op2, ";\n");
            break;
        case IMM_REG:
            OUT(c, locp, "tcg_gen_addi_", bit_suffix,
                "(", &res, ", ", &op2, ", ", &op1, ");\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_addi_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_REG:
            OUT(c, locp, "tcg_gen_add_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case SUB_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ", &res,
                " = ", &op1, " - ", &op2, ";\n");
            break;
        case IMM_REG:
            OUT(c, locp, "tcg_gen_subfi_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_subi_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_REG:
            OUT(c, locp, "tcg_gen_sub_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case MUL_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int64_t ", &res, " = ", &op1, " * ", &op2, ";\n");
            break;
        case IMM_REG:
            OUT(c, locp, "tcg_gen_muli_", bit_suffix,
                "(", &res, ", ", &op2, ", (int64_t)", &op1, ");\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_muli_", bit_suffix,
                "(", &res, ", ", &op1, ", (int64_t)", &op2, ");\n");
            break;
        case REG_REG:
            OUT(c, locp, "tcg_gen_mul_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case DIV_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int64_t ", &res, " = ", &op1, " / ", &op2, ";\n");
            break;
        case IMM_REG:
        case REG_IMM:
        case REG_REG:
            OUT(c, locp, &res, " = gen_helper_divu("
                "cpu_env, ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case ASL_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ", &res,
                " = ", &op1, " << ", &op2, ";\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_shli_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case IMM_REG:
            op1.bit_width = bit_width;
            rvalue_materialize(c, locp, &op1);
            /* Fallthrough */
        case REG_REG:
            OUT(c, locp, "tcg_gen_shl_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        if (op_types != IMM_IMM) {
            /* Handle left shift by 64 which hexagon-sim expects to clear out */
            /* register */
            t_hex_value edge = gen_tmp_value(c, locp, "64", bit_width);
            t_hex_value zero = gen_tmp_value(c, locp, "0", bit_width);
            if (op_is64bit) {
                rvalue_extend(c, locp, &op2);
            }
            rvalue_materialize(c, locp, &op1);
            rvalue_materialize(c, locp, &op2);
            op2.is_symbol = true;
            rvalue_materialize(c, locp, &edge);
            OUT(c, locp, "tcg_gen_movcond_i", &bit_width);
            if (op_types == REG_REG || op_types == IMM_REG) {
                OUT(c, locp, "(TCG_COND_EQ, ", &res, ", ", &op2, ", ", &edge);
            } else {
                OUT(c, locp, "(TCG_COND_EQ, ", &res, ", ", &op2, ", ", &edge);
            }
            OUT(c, locp, ", ", &zero, ", ", &res, ");\n");
            rvalue_free(c, locp, &edge);
            rvalue_free(c, locp, &zero);
        }
        break;
    }
    case ASR_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ", &res,
                " = ", &op1, " >> ", &op2, ";\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_sari_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case IMM_REG:
            rvalue_materialize(c, locp, &op1);
            /* Fallthrough */
        case REG_REG:
            OUT(c, locp, "tcg_gen_sar_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case LSR_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ",
                &res, " = ", &op1, " >> ", &op2, ";\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_shri_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case IMM_REG:
            rvalue_materialize(c, locp, &op1);
            /* Fallthrough */
        case REG_REG:
            OUT(c, locp, "tcg_gen_shr_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case ANDB_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ",
                &res, " = ", &op1, " & ", &op2, ";\n");
            break;
        case IMM_REG:
            OUT(c, locp, "tcg_gen_andi_", bit_suffix,
                "(", &res, ", ", &op2, ", ", &op1, ");\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_andi_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_REG:
            OUT(c, locp, "tcg_gen_and_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case ORB_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ",
                &res, " = ", &op1, " & ", &op2, ";\n");
            break;
        case IMM_REG:
            OUT(c, locp, "tcg_gen_ori_", bit_suffix,
                "(", &res, ", ", &op2, ", ", &op1, ");\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_ori_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_REG:
            OUT(c, locp, "tcg_gen_or_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case XORB_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ",
                &res, " = ", &op1, " & ", &op2, ";\n");
            break;
        case IMM_REG:
            OUT(c, locp, "tcg_gen_xori_", bit_suffix,
                "(", &res, ", ", &op2, ", ", &op1, ");\n");
            break;
        case REG_IMM:
            OUT(c, locp, "tcg_gen_xori_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_REG:
            OUT(c, locp, "tcg_gen_xor_", bit_suffix,
                "(", &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case ANDL_OP:
    {
        t_hex_value zero, tmp1, tmp2;
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ",
                &res, " = ", &op1, " && ", &op2, ";\n");
            break;
        case IMM_REG:
            zero = gen_tmp_value(c, locp, "0", 32);
            tmp2 = gen_bin_cmp_ext(c, locp, "TCG_COND_NE", &op2, &zero, false);
            OUT(c, locp, "tcg_gen_andi_", bit_suffix,
                "(", &res, ", ", &op1, " != 0 , ", &tmp2, ");\n");
            rvalue_free(c, locp, &zero);
            rvalue_free(c, locp, &tmp2);
            break;
        case REG_IMM:
            zero = gen_tmp_value(c, locp, "0", 32);
            tmp1 = gen_bin_cmp_ext(c, locp, "TCG_COND_NE", &op1, &zero, false);
            OUT(c, locp, "tcg_gen_andi_", bit_suffix,
                "(", &res, ", ", &tmp1, ", ", &op2, " != 0);\n");
            rvalue_free(c, locp, &zero);
            rvalue_free(c, locp, &tmp1);
            break;
        case REG_REG:
            zero = gen_tmp_value(c, locp, "0", 32);
            tmp1 = gen_bin_cmp_ext(c, locp, "TCG_COND_NE", &op1, &zero, false);
            tmp2 = gen_bin_cmp_ext(c, locp, "TCG_COND_NE", &op2, &zero, false);
            OUT(c, locp, "tcg_gen_and_", bit_suffix,
                "(", &res, ", ", &tmp1, ", ", &tmp2, ");\n");
            rvalue_free(c, locp, &zero);
            rvalue_free(c, locp, &tmp1);
            rvalue_free(c, locp, &tmp2);
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;

    }
    case MINI_OP:
    {
        const char *comparison = res.is_unsigned
                                 ? "TCG_COND_LEU"
                                 : "TCG_COND_LE";
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ", &res, " = (", &op1, " <= ");
            OUT(c, locp, &op2, ") ? ", &op1, " : ", &op2, ";\n");
            break;
        case IMM_REG:
            op1.bit_width = bit_width;
            rvalue_materialize(c, locp, &op1);
            OUT(c, locp, "tcg_gen_movcond_i", &bit_width);
            OUT(c, locp, "(", comparison, ", ", &res, ", ", &op1, ", ", &op2);
            OUT(c, locp, ", ", &op1, ", ", &op2, ");\n");
            break;
        case REG_IMM:
            op2.bit_width = bit_width;
            rvalue_materialize(c, locp, &op2);
            /* Fallthrough */
        case REG_REG:
            OUT(c, locp, "tcg_gen_movcond_i", &bit_width);
            OUT(c, locp, "(", comparison, ", ", &res, ", ", &op1, ", ", &op2);
            OUT(c, locp, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case MAXI_OP:
    {
        const char *comparison = res.is_unsigned
                                 ? "TCG_COND_LEU"
                                 : "TCG_COND_LE";
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int", &bit_width, "_t ", &res, " = (", &op1, " <= ");
            OUT(c, locp, &op2, ") ? ", &op2, " : ", &op1, ";\n");
            break;
        case IMM_REG:
            op1.bit_width = bit_width;
            rvalue_materialize(c, locp, &op1);
            OUT(c, locp, "tcg_gen_movcond_i", &bit_width);
            OUT(c, locp, "(", comparison, ", ", &res, ", ", &op1, ", ", &op2);
            OUT(c, locp, ", ", &op2, ", ", &op1, ");\n");
            break;
        case REG_IMM:
            op2.bit_width = bit_width;
            rvalue_materialize(c, locp, &op2);
            /* Fallthrough */
        case REG_REG:
            OUT(c, locp, "tcg_gen_movcond_i", &bit_width);
            OUT(c, locp, "(", comparison, ", ", &res, ", ", &op1, ", ", &op2);
            OUT(c, locp, ", ", &op2, ", ", &op1, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    case MOD_OP:
    {
        switch (op_types) {
        case IMM_IMM:
            OUT(c, locp, "int64_t ", &res, " = ", &op1, " % ", &op2, ";\n");
            break;
        case IMM_REG:
        case REG_IMM:
        case REG_REG:
            OUT(c, locp, "gen_helper_mod(",
                &res, ", ", &op1, ", ", &op2, ");\n");
            break;
        default:
            fprintf(stderr, "Error in evalutating immediateness!");
            abort();
        }
        break;
    }
    }
    /* Free operands only if they are unnamed */
    if (!op1.is_symbol) {
        rvalue_free(c, locp, &op1);
    }
    if (!op2.is_symbol) {
        rvalue_free(c, locp, &op2);
    }
    if (op_types == IMM_IMM) {
        c->inst.qemu_tmp_count++;
    }
    return res;

#undef IMM_IMM
#undef IMM_REG
#undef REG_IMM
#undef REG_REG
}

t_hex_value gen_cast_op(context_t *c,
                        YYLTYPE *locp,
                        t_hex_value *source,
                        unsigned target_width) {
    if (source->bit_width == target_width) {
        return *source;
    } else if (source->type == IMMEDIATE) {
        source->bit_width = target_width;
        return *source;
    } else {
        t_hex_value res = gen_tmp(c, locp, target_width);
        /* Truncate */
        if (source->bit_width > target_width) {
            OUT(c, locp, "tcg_gen_trunc_i64_tl(", &res, ", ", source, ");\n");
        } else {
            if (source->is_unsigned) {
                /* Extend unsigned */
                OUT(c, locp, "tcg_gen_extu_i32_i64(",
                    &res, ", ", source, ");\n");
            } else {
                /* Extend signed */
                OUT(c, locp, "tcg_gen_ext_i32_i64(",
                    &res, ", ", source, ");\n");
            }
        }
        rvalue_free(c, locp, source);
        return res;
    }
}

t_hex_value gen_extend_op(context_t *c,
                          YYLTYPE *locp,
                          t_hex_value *src_width,
                          t_hex_value *dst_width,
                          t_hex_value *value,
                          bool is_unsigned) {
    /* Select destination TCGv type, if destination > 32 then tcgv = 64 */
    int op_width = (dst_width->imm.value > 32) ? 64 : 32;
    t_hex_value res = gen_tmp(c, locp, op_width);
    /* Cast and materialize immediate operands and source value */
    *value = gen_cast_op(c, locp, value, op_width);
    rvalue_materialize(c, locp, value);
    /* Shift left of tcgv width - source width */
    OUT(c, locp, "tcg_gen_shli_i", &op_width, "(", &res, ", ", value);
    OUT(c, locp, ", ", &op_width, " - ", src_width, ");\n");
    /* Shift Right (arithmetic if sign extension, logic if zero extension) */
    if (is_unsigned) {
        OUT(c, locp, "tcg_gen_shri_i", &op_width, "(", &res, ", ", &res);
        OUT(c, locp, ", ", &op_width, " - ", src_width, ");\n");
    } else {
        OUT(c, locp, "tcg_gen_sari_i", &op_width, "(", &res, ", ", &res);
        OUT(c, locp, ", ", &op_width, " - ", src_width, ");\n");
    }
    /* Zero-out unwanted bits */
    if (dst_width->imm.value != op_width) {
        t_hex_value one = gen_tmp_value(c, locp, "1", op_width);
        t_hex_value tmp_mask = gen_bin_op(c, locp, ASL_OP, &one, dst_width);
        one = gen_tmp_value(c, locp, "1", op_width);
        t_hex_value mask = gen_bin_op(c, locp, SUB_OP, &tmp_mask, &one);
        res = gen_bin_op(c, locp, ANDB_OP, &res, &mask);
    }
    /* Set destination signedness */
    res.is_unsigned = is_unsigned;
    rvalue_free(c, locp, src_width);
    rvalue_free(c, locp, dst_width);
    rvalue_free(c, locp, value);
    return res;
}

void gen_rdeposit_op(context_t *c,
                     YYLTYPE *locp,
                     t_hex_value *dest,
                     t_hex_value *value,
                     t_hex_range *range)
{
    int bit_width = (dest->bit_width == 64) ? 64 : 32;
    int begin = range->begin;
    int end = range->end;
    int width = end - begin + 1;
    /* If the destination value is 32, truncate the value, otherwise extend */
    if (dest->bit_width == 32) {
        rvalue_truncate(c, locp, value);
    } else {
        rvalue_extend(c, locp, value);
    }
    if (dest->type == PREDICATE) {
        /* If predicate, apply (bool) ? 0xff : 0x00 */
        t_hex_value tmp = gen_tmp(c, locp, 32);
        t_hex_value zero = gen_tmp_value(c, locp, "0x0", 32);
        t_hex_value ff = gen_tmp_value(c, locp, "0xff", 32);
        OUT(c, locp, "tcg_gen_movcond_i32");
        OUT(c, locp, "(TCG_COND_EQ, ", &tmp, ", ", value, ", ", &zero);
        OUT(c, locp, ", ", &zero, ", ", &ff, ");\n");
        *value = tmp;
    }
    rvalue_materialize(c, locp, value);
    OUT(c, locp, "tcg_gen_deposit_i", &bit_width, "(", dest, ", ", dest, ", ");
    OUT(c, locp, value, ", ", &begin, ", ", &width, ");\n");
    rvalue_free(c, locp, value);
}

void gen_deposit_op(context_t *c,
                    YYLTYPE *locp,
                    t_hex_value *dest,
                    t_hex_value *value,
                    t_hex_value *index,
                    t_hex_cast *cast)
{
    yyassert(c, locp, index->type == IMMEDIATE,
             "Deposit index must be immediate!\n");
    int bit_width = (dest->bit_width == 64) ? 64 : 32;
    int width = cast->bit_width;
    /* If the destination value is 32, truncate the value, otherwise extend */
    if (dest->bit_width != value->bit_width) {
        if (bit_width == 32) {
            rvalue_truncate(c, locp, value);
        } else {
            rvalue_extend(c, locp, value);
        }
    }
    rvalue_materialize(c, locp, value);
    OUT(c, locp, "tcg_gen_deposit_i", &bit_width, "(", dest, ", ", dest, ", ");
    OUT(c, locp, value, ", ", index, " * ", &width, ", ", &width, ");\n");
    rvalue_free(c, locp, value);
}

t_hex_value gen_rextract_op(context_t *c,
                           YYLTYPE *locp,
                           t_hex_value *source,
                           t_hex_range *range) {
    int bit_width = (source->bit_width == 64) ? 64 : 32;
    int begin = range->begin;
    int end = range->end;
    int width = end - begin + 1;
    const char *sign_prefix = (range->is_unsigned) ? "" : "s";
    t_hex_value res = gen_tmp(c, locp, bit_width);
    OUT(c, locp, "tcg_gen_", sign_prefix, "extract_i", &bit_width, "(", &res);
    OUT(c, locp, ", ", source, ", ", &begin, ", ", &width, ");\n");
    *source = res;
    return *source;
}

t_hex_value gen_extract_op(context_t *c,
                           YYLTYPE *locp,
                           t_hex_value *source,
                           t_hex_value *index,
                           t_hex_extract *extract) {
    yyassert(c, locp, index->type == IMMEDIATE,
             "Extract index must be immediate!\n");
    int bit_width = (source->bit_width == 64) ? 64 : 32;
    const char *sign_prefix = (extract->is_unsigned) ? "" : "s";
    int width = extract->bit_width;
    t_hex_value res = gen_tmp(c, locp, bit_width);
    res.is_unsigned = extract->is_unsigned;
    OUT(c, locp, "tcg_gen_", sign_prefix, "extract_i", &bit_width,
        "(", &res, ", ", source);
    OUT(c, locp, ", ", index, " * ", &width, ", ", &width, ");\n");

    /* Some extract operations have bit_width != storage_bit_width */
    if (extract->storage_bit_width > bit_width) {
        t_hex_value tmp = gen_tmp(c, locp, extract->storage_bit_width);
        tmp.is_unsigned = extract->is_unsigned;
        if (extract->is_unsigned) {
            /* Extend unsigned */
            OUT(c, locp, "tcg_gen_extu_i32_i64(",
                &tmp, ", ", &res, ");\n");
        } else {
            /* Extend signed */
            OUT(c, locp, "tcg_gen_ext_i32_i64(",
                &tmp, ", ", &res, ");\n");
        }
        rvalue_free(c, locp, &res);
        res = tmp;
    }

    *source = res;
    return *source;
}

t_hex_value gen_read_creg(context_t *c, YYLTYPE *locp, t_hex_value *reg)
{
    if (reg->reg.type == CONTROL) {
        t_hex_value tmp = gen_tmp_value(c, locp, "0", 32);
        const char *id = creg_str[(uint8_t)reg->reg.id];
        OUT(c, locp, "READ_REG(", &tmp, ", ", id, ");\n");
        return tmp;
    }
    return *reg;
}

void gen_write_creg(context_t *c,
                    YYLTYPE *locp,
                    t_hex_value *reg,
                    t_hex_value *value)
{
    rvalue_truncate(c, locp, value);
    rvalue_materialize(c, locp, value);
    OUT(c,
        locp,
        "gen_log_reg_write(", creg_str[(uint8_t)reg->reg.id], ", ",
        value, ");\n");
    OUT(c,
        locp,
        "ctx_log_reg_write(ctx, ", creg_str[(uint8_t)reg->reg.id], ");\n");
    rvalue_free(c, locp, value);
}

void gen_assign(context_t *c,
                YYLTYPE *locp,
                t_hex_value *dest,
                t_hex_value *value)
{
    if (dest->type == REGISTER && dest->reg.type == CONTROL) {
        gen_write_creg(c, locp, dest, value);
        return;
    }
    /* Create (if not present) and assign to temporary variable */
    if (dest->type == VARID) {
        varid_allocate(c, locp, dest, value->bit_width, value->is_unsigned);
    }
    int bit_width = dest->bit_width == 64 ? 64 : 32;
    if (bit_width != value->bit_width) {
        if (bit_width == 64) {
            rvalue_extend(c, locp, value);
        } else {
            rvalue_truncate(c, locp, value);
        }
    }
    rvalue_materialize(c, locp, value);
    if (value->type == IMMEDIATE) {
        OUT(c, locp, "tcg_gen_movi_i", &bit_width,
            "(", dest, ", ", value, ");\n");
    } else {
        OUT(c, locp, "tcg_gen_mov_i", &bit_width,
            "(", dest, ", ", value, ");\n");
    }
    rvalue_free(c, locp, value);
}

t_hex_value gen_convround(context_t *c,
                          YYLTYPE *locp,
                          t_hex_value *source)
{
    unsigned bit_width = source->bit_width;
    const char *size = (bit_width == 32) ? "32" : "64";
    t_hex_value res = gen_tmp(c, locp, bit_width);

    t_hex_value mask = gen_tmp_value(c, locp, "0x3", bit_width);
    mask.is_symbol = true;
    source->is_symbol = true;
    t_hex_value and = gen_bin_op(c, locp, ANDB_OP, source, &mask);
    t_hex_value one = gen_tmp_value(c, locp, "1", bit_width);
    t_hex_value source_p1 = gen_bin_op(c, locp, ADD_OP, source, &one);

    OUT(c, locp, "tcg_gen_movcond_i", size, "(TCG_COND_EQ, ", &res);
    OUT(c, locp, ", ", &and, ", ", &mask, ", ");
    OUT(c, locp, &source_p1, ", ", source, ");\n");

    rvalue_free(c, locp, &mask);
    rvalue_free(c, locp, source);

    return res;
}

static t_hex_value gen_convround_n_a(context_t *c,
                                     YYLTYPE *locp,
                                     t_hex_value *a,
                                     t_hex_value *n)
{
    t_hex_value res = gen_tmp(c, locp, 64);
    OUT(c, locp, "tcg_gen_ext_i32_i64(", &res, ", ", a, ");\n");
    return res;
}

static t_hex_value gen_convround_n_b(context_t *c,
                                     YYLTYPE *locp,
                                     t_hex_value *a,
                                     t_hex_value *n)
{
    t_hex_value res = gen_tmp(c, locp, 64);
    OUT(c, locp, "tcg_gen_ext_i32_i64(", &res, ", ", a, ");\n");

    t_hex_value one = gen_tmp_value(c, locp, "1", 32);
    t_hex_value tmp = gen_tmp(c, locp, 32);
    t_hex_value tmp_64 = gen_tmp(c, locp, 64);

    OUT(c, locp, "tcg_gen_shl_i32(", &tmp);
    OUT(c, locp, ", ", &one, ", ", n, ");\n");
    OUT(c, locp, "tcg_gen_and_i32(", &tmp);
    OUT(c, locp, ", ", &tmp, ", ", a, ");\n");
    OUT(c, locp, "tcg_gen_shri_i32(", &tmp);
    OUT(c, locp, ", ", &tmp, ", 1);\n");
    OUT(c, locp, "tcg_gen_ext_i32_i64(", &tmp_64, ", ", &tmp, ");\n");
    OUT(c, locp, "tcg_gen_add_i64(", &res);
    OUT(c, locp, ", ", &res, ", ", &tmp_64, ");\n");

    rvalue_free(c, locp, &one);
    rvalue_free(c, locp, &tmp);
    rvalue_free(c, locp, &tmp_64);

    return res;
}

static t_hex_value gen_convround_n_c(context_t *c,
                                     YYLTYPE *locp,
                                     t_hex_value *a,
                                     t_hex_value *n)
{
    t_hex_value res = gen_tmp(c, locp, 64);
    OUT(c, locp, "tcg_gen_ext_i32_i64(", &res, ", ", a, ");\n");

    t_hex_value one = gen_tmp_value(c, locp, "1", 32);
    t_hex_value tmp = gen_tmp(c, locp, 32);
    t_hex_value tmp_64 = gen_tmp(c, locp, 64);

    OUT(c, locp, "tcg_gen_subi_i32(", &tmp);
    OUT(c, locp, ", ", n, ", 1);\n");
    OUT(c, locp, "tcg_gen_shl_i32(", &tmp);
    OUT(c, locp, ", ", &one, ", ", &tmp, ");\n");
    OUT(c, locp, "tcg_gen_ext_i32_i64(", &tmp_64, ", ", &tmp, ");\n");
    OUT(c, locp, "tcg_gen_add_i64(", &res);
    OUT(c, locp, ", ", &res, ", ", &tmp_64, ");\n");

    rvalue_free(c, locp, &one);
    rvalue_free(c, locp, &tmp);
    rvalue_free(c, locp, &tmp_64);

    return res;
}

t_hex_value gen_convround_n(context_t *c,
                            YYLTYPE *locp,
                            t_hex_value *source,
                            t_hex_value *bit_pos)
{
    /* If input is 64 bit cast it to 32 */
    t_hex_value a = gen_cast_op(c, locp, source, 32);
    t_hex_value n = gen_cast_op(c, locp, bit_pos, 32);

    rvalue_materialize(c, locp, &a);
    rvalue_materialize(c, locp, &n);

    t_hex_value r1 = gen_convround_n_a(c, locp, &a, &n);
    t_hex_value r2 = gen_convround_n_b(c, locp, &a, &n);
    t_hex_value r3 = gen_convround_n_c(c, locp, &a, &n);

    t_hex_value l_32 = gen_tmp_value(c, locp, "1", 32);

    t_hex_value cond = gen_tmp(c, locp, 32);
    t_hex_value cond_64 = gen_tmp(c, locp, 64);
    t_hex_value mask = gen_tmp(c, locp, 32);
    t_hex_value n_64 = gen_tmp(c, locp, 64);
    t_hex_value res = gen_tmp(c, locp, 64);
    t_hex_value zero = gen_tmp_value(c, locp, "0", 64);

    OUT(c, locp, "tcg_gen_sub_i32(", &mask);
    OUT(c, locp, ", ", &n, ", ", &l_32, ");\n");
    OUT(c, locp, "tcg_gen_shl_i32(", &mask);
    OUT(c, locp, ", ", &l_32, ", ", &mask, ");\n");
    OUT(c, locp, "tcg_gen_sub_i32(", &mask);
    OUT(c, locp, ", ", &mask, ", ", &l_32, ");\n");
    OUT(c, locp, "tcg_gen_and_i32(", &cond);
    OUT(c, locp, ", ", &a, ", ", &mask, ");\n");
    OUT(c, locp, "tcg_gen_extu_i32_i64(", &cond_64, ", ", &cond, ");\n");
    OUT(c, locp, "tcg_gen_ext_i32_i64(", &n_64, ", ", &n, ");\n");

    OUT(c, locp, "tcg_gen_movcond_i64");
    OUT(c, locp, "(TCG_COND_EQ, ", &res, ", ", &cond_64, ", ", &zero);
    OUT(c, locp, ", ", &r2, ", ", &r3, ");\n");

    OUT(c, locp, "tcg_gen_movcond_i64");
    OUT(c, locp, "(TCG_COND_EQ, ", &res, ", ", &n_64, ", ", &zero);
    OUT(c, locp, ", ", &r1, ", ", &res, ");\n");

    OUT(c, locp, "tcg_gen_shr_i64(", &res);
    OUT(c, locp, ", ", &res, ", ", &n_64, ");\n");

    rvalue_free(c, locp, &a);
    rvalue_free(c, locp, &n);

    rvalue_free(c, locp, &r1);
    rvalue_free(c, locp, &r2);
    rvalue_free(c, locp, &r3);

    rvalue_free(c, locp, &cond);
    rvalue_free(c, locp, &cond_64);
    rvalue_free(c, locp, &l_32);
    rvalue_free(c, locp, &mask);
    rvalue_free(c, locp, &n_64);
    rvalue_free(c, locp, &zero);

    rvalue_truncate(c, locp, &res);

    return res;
}

t_hex_value gen_round(context_t *c,
                      YYLTYPE *locp,
                      t_hex_value *source,
                      t_hex_value *position) {
    yyassert(c, locp, source->bit_width <= 32,
             "fRNDN not implemented for bit widths > 32!");

    t_hex_value src = *source;
    t_hex_value pos = *position;

    t_hex_value src_width = gen_imm_value(c, locp, src.bit_width, 32);
    t_hex_value dst_width = gen_imm_value(c, locp, 64, 32);
    t_hex_value a = gen_extend_op(c, locp, &src_width, &dst_width, &src, false);

    src_width = gen_imm_value(c, locp, 5, 32);
    dst_width = gen_imm_value(c, locp, 64, 32);
    t_hex_value b = gen_extend_op(c, locp, &src_width, &dst_width, &pos, true);

    // Disable auto-free of values used more than once
    a.is_symbol = true;
    b.is_symbol = true;

    t_hex_value res = gen_tmp(c, locp, 64);

    t_hex_value one = gen_tmp_value(c, locp, "1", 64);
    t_hex_value n_m1 = gen_bin_op(c, locp, SUB_OP, &b, &one);
    one = gen_tmp_value(c, locp, "1", 64);
    t_hex_value shifted = gen_bin_op(c, locp, ASL_OP, &one, &n_m1);
    t_hex_value sum = gen_bin_op(c, locp, ADD_OP, &shifted, &a);

    t_hex_value zero = gen_tmp_value(c, locp, "0", 64);
    OUT(c, locp, "tcg_gen_movcond_i64");
    OUT(c, locp, "(TCG_COND_EQ, ", &res, ", ", &b, ", ", &zero);
    OUT(c, locp, ", ", &a, ", ", &sum, ");\n");

    rvalue_free(c, locp, &a);
    rvalue_free(c, locp, &b);
    rvalue_free(c, locp, &zero);
    rvalue_free(c, locp, &sum);

    return res;
}

/* Circular addressing mode with auto-increment */
t_hex_value gen_circ_op(context_t *c,
                        YYLTYPE *locp,
                        t_hex_value *addr,
                        t_hex_value *increment,
                        t_hex_value *modifier) {
    t_hex_value cs = gen_tmp(c, locp, 32);
    rvalue_materialize(c, locp, increment);
    OUT(c, locp, "READ_REG(", &cs, ", HEX_REG_CS0 + MuN);\n");
    OUT(c, locp, "gen_helper_fcircadd(", addr, ", ", addr, ", ", increment, ", ", modifier);
    OUT(c, locp, ", ", &cs, ");\n");
    rvalue_free(c, locp, &cs);
    rvalue_free(c, locp, increment);
    return *addr;
}

t_hex_value gen_bitcnt_op(context_t *c, YYLTYPE *locp, t_hex_value *source,
                          bool negate,
                          bool reverse)
{
    const char *bit_suffix = source->bit_width == 64 ? "64" : "32";
    t_hex_value res = gen_tmp(c, locp, source->bit_width == 64 ? 64 : 32);
    res.type = TEMP;
    rvalue_materialize(c, locp, source);
    switch (negate << 1 | reverse) {
    case 0b00:
        OUT(c, locp, "tcg_gen_clzi_i", bit_suffix,
            "(", &res, ", ", source, ", ");
        OUT(c, locp, bit_suffix, ");");
        break;
    case 0b01:
        OUT(c, locp, "tcg_gen_ctzi_i", bit_suffix,
            "(", &res, ", ", source, ", ");
        OUT(c, locp, bit_suffix, ");");
        break;
    case 0b10:
        OUT(c, locp, "tcg_gen_not_i", bit_suffix,
            "(", &res, ", ", source, ");\n");
        OUT(c, locp, "tcg_gen_clzi_i", bit_suffix,
            "(", &res, ", ", &res, ", ");
        OUT(c, locp, bit_suffix, ");");
        break;
    case 0b11:
        OUT(c, locp, "tcg_gen_not_i", bit_suffix,
            "(", &res, ", ", source, ");\n");
        OUT(c, locp, "tcg_gen_clzi_i", bit_suffix, "(", &res, ", ", &res, ", ");
        OUT(c, locp, bit_suffix, ");");
        break;
    }
    rvalue_free(c, locp, source);
    return res;
}

t_hex_value gen_ctpop_op(context_t *c, YYLTYPE *locp, t_hex_value *source)
{
    const char *bit_suffix = source->bit_width == 64 ? "64" : "32";
    t_hex_value res = gen_tmp(c, locp, source->bit_width == 64 ? 64 : 32);
    res.type = TEMP;
    rvalue_materialize(c, locp, source);
    OUT(c, locp, "tcg_gen_ctpop_i", bit_suffix,
        "(", &res, ", ", source, ");\n");
    rvalue_free(c, locp, source);
    return res;
}

bool reg_equal(t_hex_reg *r1, t_hex_reg *r2)
{
    return !memcmp(r1, r2, sizeof(t_hex_reg));
}

bool pre_equal(t_hex_pre *p1, t_hex_pre *p2)
{
    return !memcmp(p1, p2, sizeof(t_hex_pre));
}

bool rvalue_equal(t_hex_value *v1, t_hex_value *v2)
{
    if (v1->is_dotnew != v2->is_dotnew) {
        return false;
    } else if (v1->type == REGISTER && v2->type == REGISTER) {
        return reg_equal(&(v1->reg), &(v2->reg));
    } else if (v1->type == PREDICATE && v2->type == PREDICATE) {
        return pre_equal(&(v1->pre), &(v2->pre));
    } else {
        return false;
    }
}

void emit_header(context_t *c)
{
    EMIT_SIG(c, START_COMMENT " %s " END_COMMENT "\n", c->inst.name);
    EMIT_SIG(c, "void emit_%s(DisasContext *ctx, Insn *insn, Packet *pkt",
             c->inst.name);
}

void emit_arg(context_t *c, YYLTYPE *locp, t_hex_value *arg)
{
    switch (arg->type) {
    case REGISTER:
        if (arg->reg.type == DOTNEW) {
            EMIT_SIG(c, ", TCGv N%cN", arg->reg.id);
        } else {
            bool is64 = (arg->bit_width == 64);
            const char *type = is64 ? "TCGv_i64" : "TCGv_i32";
            char reg_id[5] = { 0 };
            reg_compose(c, locp, &(arg->reg), reg_id);
            EMIT_SIG(c, ", %s %s", type, reg_id);
            /* MuV register requires also MuN to provide its index */
            if (arg->reg.type == MODIFIER) {
                EMIT_SIG(c, ", int MuN");
            }
        }
        break;
    case PREDICATE:
        {
            char suffix = arg->is_dotnew ? 'N' : 'V';
            EMIT_SIG(c, ", TCGv P%c%c", arg->pre.id, suffix);
        }
        break;
    default:
        {
            fprintf(stderr, "emit_arg got unsupported argument!");
            abort();
        }
    }
}

void emit_footer(context_t *c)
{
    EMIT(c, "}\n");
    EMIT(c, "\n");
}