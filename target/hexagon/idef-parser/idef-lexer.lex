%option noyywrap noinput nounput
%option 8bit reentrant bison-bridge
%option warn nodefault
%option header-file="idef-parser.yy.h"
%option bison-locations

%{
/*
 * Copyright(c) 2019-2020 rev.ng Srls. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdbool.h>

#include "idef-parser.h"
#include "idef-parser.tab.h"

/* Keep track of scanner position for error message printout */
#define YY_USER_ACTION yylloc->first_column = yylloc->last_column; \
    for (int i = 0; yytext[i] != '\0'; i++) {   \
        yylloc->last_column++;                  \
    }

/* Global Error Counter */
int error_count;

%}

/* Definitions */
DIGIT                    [0-9]
LOWER_ID                 [a-z]
UPPER_ID                 [A-Z]
ID                       LOWER_ID|UPPER_ID
INST_NAME                [A-Z]+[0-9]_([a-z]|[0-9]|_)+
HEX_DIGIT                [0-9a-fA-F]
REG_ID_32                e|s|d|t|u|v|x|y
REG_ID_64                ee|ss|dd|tt|uu|vv|xx|yy
SYS_ID_32                s|d
SYS_ID_64                ss|dd
LOWER_PRE                d|s|t|u|v|e|x|x
ZERO_ONE                 0|1
IMM_ID                   r|s|S|u|U
VAR_ID                   [a-zA-Z_][a-zA-Z0-9_]*
SIGN_ID                  s|u

/* Tokens */
%%

[ \t\f\v]+                { /* Ignore whitespaces. */ }
[\n\r]+                   { /* Ignore newlines. */ }

{INST_NAME}               { yylval->string = strdup(yytext);
                            return INAME; }
"RREG_"{REG_ID_32} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[5];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return DREG; }
"NEW_NREG_"{REG_ID_32} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = DOTNEW;
                           yylval->rvalue.reg.id = yytext[9];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return DREG; }
"RREG_"{REG_ID_64} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[5];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_dotnew = false;
                           return DREG; }
"NEW_NREG_"{REG_ID_64} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = DOTNEW;
                           yylval->rvalue.reg.id = yytext[10];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_dotnew = true;
                           return DREG; }
"MREG_u" {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = MODIFIER;
                           yylval->rvalue.reg.id = 'u';
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return DREG; }
"CREG_"{REG_ID_32} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = yytext[5];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return DREG; }
"IMM_"{IMM_ID}"iV" {
                           yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VARIABLE;
                           yylval->rvalue.imm.id = yytext[4];
                           yylval->rvalue.is_dotnew = false;
                           return DIMM; }
"PREG_"{LOWER_PRE} {
                           yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[5];
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return DPRE; }
"NEW_PREG_"{LOWER_PRE} {
                           yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[9];
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return DPRE; }
"ea"                       { return DEA; }
"in_RREG_"{REG_ID_32} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[8];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return RREG; }
"in_RREG_"{REG_ID_64} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[8];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_dotnew = false;
                           return RREG; }
"in_NEW_NREG_"{REG_ID_32} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = DOTNEW;
                           yylval->rvalue.reg.id = yytext[12];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return RREG; }
"in_NEW_NREG_"{REG_ID_64} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = DOTNEW;
                           yylval->rvalue.reg.id = yytext[12];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_dotnew = true;
                           return RREG; }
"in_PREG_"{LOWER_PRE} {
                           yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[8];
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return RPRE; }
"in_NEW_PREG_"{LOWER_PRE} {
                           yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[12];
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return RPRE; }
"in_MREG_u" {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = MODIFIER;
                           yylval->rvalue.reg.id = 'u';
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return RREG; }
"in_CREG_"{REG_ID_32} {
                           yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = yytext[5];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return RREG; }
"fGEN_TCG_"{INST_NAME}"(" { return FWRAP; }
"{"                      { return LBR; }
"}"                      { return RBR; }
"["                      { return LSQ; }
"]"                      { return RSQ; }
"("                      { return LPAR; }
")"                      { return RPAR; }
"IV1DEAD()"              |
"fPAUSE(uiV);"           |
";"                      { return SEMI; }
":"                      { return COLON; }
"+"                      { return PLUS; }
"-"                      { return MINUS; }
"*"                      { return MUL; }
"**"                     { return POW; }
"/"                      { return DIV; }
"%"                      { return MOD; }
"&"                      { return AND; }
"|"                      { return OR; }
"^"                      { return XOR; }
"~"                      { return NOT; }
"="                      { return ASSIGN; }
"+="                     { return INC; }
"-="                     { return DEC; }
"++"                     { return PLUSPLUS; }
"&="                     { return ANDA; }
"|="                     { return ORA; }
"^="                     { return XORA; }
"<"                      { return LT; }
">"                      { return GT; }
"<<"                     { return ASL; }
">>"                     { return ASR; }
">>>"                    { return LSR; }
"=="                     { return EQ; }
"!="                     { return NEQ; }
"<="                     { return LTE; }
">="                     { return GTE; }
"&&"                     { return ANDL; }
"||"                     { return ORL; }
"!"                      { return NOTL; }
","                      { return COMMA; }
"else"                   { return ELSE; }
"for"                    { return FOR; }
"fREAD_IREG"             { return ICIRC; }
"fPART1"                 { return PART1; }
"if"                     { return IF; }
"fFRAME_SCRAMBLE"        { return FSCR; }
"fFRAME_UNSCRAMBLE"      { return FSCR; }
"fFRAMECHECK"            { return FCHK; }
"Constant_extended"      { return CONSTEXT; }
"fCL1_"{DIGIT}           { return LOCNT; }
"fBREV_"{DIGIT}          { return BREV; }
"brev"                   { return BREV; }
"fSXTN"                  { return SXT; }
"fZXTN"                  { return ZXT; }
"fDF_MAX"                |
"fSF_MAX"                |
"fMAX"                   { return MAX; }
"fDF_MIN"                |
"fSF_MIN"                |
"fMIN"                   { return MIN; }
"fABS"                   { return ABS; }
"fRNDN"                  { return ROUND; }
"fCRND"                  { return CROUND; }
"fCRNDN"                 { return CROUND; }
"fPM_CIRI"               { return CIRCADD; }
"fPM_CIRR"               { return CIRCADD; }
"fCOUNTONES_"{DIGIT}     { return COUNTONES; }
"fSATN"                  { yylval->sat.set_overflow = false;
                           yylval->sat.is_unsigned = false;
                           return SAT; }
"fVSATN"                 { yylval->sat.set_overflow = true;
                           yylval->sat.is_unsigned = false;
                           return SAT; }
"fSATUN"                 { yylval->sat.set_overflow = false;
                           yylval->sat.is_unsigned = true;
                           return SAT; }
"fVSATUN"                { yylval->sat.set_overflow = true;
                           yylval->sat.is_unsigned = true;
                           return SAT; }
"fSE32_64"               { yylval->cast.bit_width = 64;
                           yylval->cast.is_unsigned = false;
                           return CAST; }
"fCAST4u"                { yylval->cast.bit_width = 32;
                           yylval->cast.is_unsigned = true;
                           return CAST; }
"fCAST4s"                { yylval->cast.bit_width = 32;
                           yylval->cast.is_unsigned = false;
                           return CAST; }
"fCAST8u"                { yylval->cast.bit_width = 64;
                           yylval->cast.is_unsigned = true;
                           return CAST; }
"fCAST8s"                { yylval->cast.bit_width = 64;
                           yylval->cast.is_unsigned = false;
                           return CAST; }
"fGETBIT"                { yylval->extract.bit_width = 1;
                           yylval->extract.storage_bit_width = 1;
                           yylval->extract.is_unsigned = false;
                           return EXTRACT; }
"fGETBYTE"               { yylval->extract.bit_width = 8;
                           yylval->extract.storage_bit_width = 8;
                           yylval->extract.is_unsigned = false;
                           return EXTRACT; }
"fGETUBYTE"              { yylval->extract.bit_width = 8;
                           yylval->extract.storage_bit_width = 8;
                           yylval->extract.is_unsigned = true;
                           return EXTRACT; }
"fGETHALF"               { yylval->extract.bit_width = 16;
                           yylval->extract.storage_bit_width = 16;
                           yylval->extract.is_unsigned = false;
                           return EXTRACT; }
"fGETUHALF"              { yylval->extract.bit_width = 16;
                           yylval->extract.storage_bit_width = 16;
                           yylval->extract.is_unsigned = true;
                           return EXTRACT; }
"fGETWORD"               { yylval->extract.bit_width = 32;
                           yylval->extract.storage_bit_width = 64;
                           yylval->extract.is_unsigned = false;
                           return EXTRACT; }
"fGETUWORD"              { yylval->extract.bit_width = 32;
                           yylval->extract.storage_bit_width = 64;
                           yylval->extract.is_unsigned = true;
                           return EXTRACT; }
"fEXTRACTU_BITS"         |
"fEXTRACTU_BIDIR"        { yylval->range.is_unsigned = true;
                           return EXTBITS; }
"fEXTRACTU_RANGE"        { yylval->range.is_unsigned = true;
                           return EXTRANGE; }
"fSETBIT"                { yylval->cast.bit_width = 1;
                           yylval->cast.is_unsigned = false;
                           return DEPOSIT; }
"fSETBYTE"               { yylval->cast.bit_width = 8;
                           yylval->cast.is_unsigned = false;
                           return DEPOSIT; }
"fSETHALF"               { yylval->cast.bit_width = 16;
                           yylval->cast.is_unsigned = false;
                           return SETHALF; }
"fSETWORD"               { yylval->cast.bit_width = 32;
                           yylval->cast.is_unsigned = false;
                           return DEPOSIT; }
"fINSERT_BITS"           { return INSBITS; }
"fINSERT_RANGE"          { return INSRANGE; }
"fSETBITS"               { return SETBITS; }
"fMPY8UU"                { yylval->mpy.first_bit_width = 8;
                           yylval->mpy.second_bit_width = 8;
                           yylval->mpy.first_unsigned = true;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fMPY8US"                { yylval->mpy.first_bit_width = 8;
                           yylval->mpy.second_bit_width = 8;
                           yylval->mpy.first_unsigned = true;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY8SU"                { yylval->mpy.first_bit_width = 8;
                           yylval->mpy.second_bit_width = 8;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fMPY8SS"                { yylval->mpy.first_bit_width = 8;
                           yylval->mpy.second_bit_width = 8;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY16UU"               { yylval->mpy.first_bit_width = 16;
                           yylval->mpy.second_bit_width = 16;
                           yylval->mpy.first_unsigned = true;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fMPY16US"               { yylval->mpy.first_bit_width = 16;
                           yylval->mpy.second_bit_width = 16;
                           yylval->mpy.first_unsigned = true;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY16SU"               { yylval->mpy.first_bit_width = 16;
                           yylval->mpy.second_bit_width = 16;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fMPY16SS"               { yylval->mpy.first_bit_width = 16;
                           yylval->mpy.second_bit_width = 16;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY32UU"               { yylval->mpy.first_bit_width = 32;
                           yylval->mpy.second_bit_width = 32;
                           yylval->mpy.first_unsigned = true;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fMPY32US"               { yylval->mpy.first_bit_width = 32;
                           yylval->mpy.second_bit_width = 32;
                           yylval->mpy.first_unsigned = true;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY32SU"               { yylval->mpy.first_bit_width = 32;
                           yylval->mpy.second_bit_width = 32;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fSFMPY"                 |
"fMPY32SS"               { yylval->mpy.first_bit_width = 32;
                           yylval->mpy.second_bit_width = 32;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY3216SS"             { yylval->mpy.first_bit_width = 32;
                           yylval->mpy.second_bit_width = 16;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = false;
                           return MPY; }
"fMPY3216SU"             { yylval->mpy.first_bit_width = 32;
                           yylval->mpy.second_bit_width = 16;
                           yylval->mpy.first_unsigned = false;
                           yylval->mpy.second_unsigned = true;
                           return MPY; }
"fNEWREG"                |
"fNEWREG_ST"             |
"fIMMEXT"                |
"fMUST_IMMEXT"           |
"fCAST2_2s"              |
"fCAST2_2u"              |
"fCAST4_4s"              |
"fCAST4_4u"              |
"fCAST8_8s"              |
"fCAST8_8u"              |
"fZE8_16"                |
"fSE8_16"                |
"fZE16_32"               |
"fSE16_32"               |
"fZE32_64"               |
"fPASS"                  |
"fFLOAT"                 |
"fUNLOAT"                |
"fDOUBLE"                |
"fUNDOUBLE"              |
"fECHO"                  { return IDENTITY; }
"(size8"[us]"_t)"        { yylval->cast.bit_width = 8;
                           yylval->cast.is_unsigned = ((yytext[6]) == 'u');
                           return CAST; }
"(size16"[us]"_t)"       { yylval->cast.bit_width = 16;
                           yylval->cast.is_unsigned = ((yytext[7]) == 'u');
                           return CAST; }
"(int)"                  { yylval->cast.bit_width = 32;
                           yylval->cast.is_unsigned = false;
                           return CAST; }
"?"                      { return QMARK; }
"fREAD_PC()"             |
"PC"                     { return PC; }
"fREAD_NPC()"            |
"NPC"                    { return NPC; }
"fGET_LPCFG"             |
"USR.LPCFG"              { return LPCFG; }
"LOAD_CANCEL(EA)"        |
"STORE_CANCEL(EA)"       |
"CANCEL"                 { return CANC; }
"R"{REG_ID_32}"V"        { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"C"{REG_ID_32}           { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"C"{REG_ID_64}           { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           return REG; }
"R"{REG_ID_64}"V"        { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           return REG; }
"N"{LOWER_ID}            { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"N"{LOWER_ID}"N"         { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = DOTNEW;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"S"{SYS_ID_32}           { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = SYSTEM;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"S"{SYS_ID_64}           { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = SYSTEM;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 64;
                           yylval->rvalue.bit_width = 64;
                           return REG; }
[rR]{DIGIT}+             { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = GENERAL_PURPOSE;
                           yylval->rvalue.reg.id = atoi(yytext + 1);
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"SGP"{DIGIT}             { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = SYSTEM;
                           yylval->rvalue.reg.id = atoi(yytext + 3);
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"fREAD_SP()"             |
"SP"                     { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = SP;
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"fREAD_FP()"             |
"FP"                     { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = FP;
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"fREAD_LR()"             |
"LR"                     { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = LR;
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"GP"                     { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = GP;
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"fREAD_LC"{ZERO_ONE}     { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = LC0 + atoi(yytext + 8);
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"LC"{ZERO_ONE}           { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = LC0 + atoi(yytext + 2);
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"fREAD_SA"{ZERO_ONE}     { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = SA0 + atoi(yytext + 8);
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"SA"{ZERO_ONE}           { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = CONTROL;
                           yylval->rvalue.reg.id = SA0 + atoi(yytext + 2);
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"MuV"                    { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = MODIFIER;
                           yylval->rvalue.reg.id = yytext[1];
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"MuN"                    { return MUN; }
"ELR"                    { yylval->rvalue.type = REGISTER;
                           yylval->rvalue.reg.type = SYSTEM;
                           yylval->rvalue.reg.id = 3;
                           yylval->rvalue.reg.bit_width = 32;
                           yylval->rvalue.bit_width = 32;
                           return REG; }
"fREAD_P0()"             { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = '0';
                           yylval->rvalue.bit_width = 32;
                           return PRE; }
[pP]{DIGIT}              { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[1];
                           yylval->rvalue.bit_width = 32;
                           return PRE; }
"fLSBNEW(P"{LOWER_PRE}"N)" { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[9];
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return PRE; }
"fLSBNEW0"               { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = '0';
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return PRE; }
"fLSBNEW1"               { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = '1';
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return PRE; }
"fLSBNEW1NOT"            { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = '1';
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = true;
                           return PRE; }
"P"{LOWER_PRE}V          { yylval->rvalue.type = PREDICATE;
                           yylval->rvalue.pre.id = yytext[1];
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_dotnew = false;
                           return PRE; }
{IMM_ID}"iV"             { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VARIABLE;
                           yylval->rvalue.imm.id = yytext[0];
                           return IMM; }
"N"                      { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.imm.type = VARIABLE;
                           yylval->rvalue.imm.id = 'N';
                           return IMM; }
"i"                      { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.imm.type = I;
                           return IMM; }
{SIGN_ID}                { yylval->is_unsigned = (yytext[0] == 'u');
                           return SIGN;
                         }
"fSF_BIAS()"             { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VALUE;
                           yylval->rvalue.imm.value = 127;
                           return IMM; }
{DIGIT}+                 { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VALUE;
                           yylval->rvalue.imm.value = atoi(yytext);
                           return IMM; }
{DIGIT}+"LL"             { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VALUE;
                           yylval->rvalue.imm.value = atoi(yytext);
                           return IMM; }
"0x"{HEX_DIGIT}+         { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 32;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VALUE;
                           yylval->rvalue.imm.value = strtol(yytext, NULL, 16);
                           return IMM; }
"0x"{HEX_DIGIT}+"LL"     { yylval->rvalue.type = IMMEDIATE;
                           yylval->rvalue.bit_width = 64;
                           yylval->rvalue.is_unsigned = false;
                           yylval->rvalue.imm.type = VALUE;
                           yylval->rvalue.imm.value = strtol(yytext, NULL, 16);
                           return IMM; }
"fCONSTLL"               { return CONSTLL; }
"fCONSTULL"              { return CONSTULL; }
"fLOAD"                  { return LOAD; }
"fSTORE"                 { return STORE; }
{VAR_ID}                 { /* Variable name, we adopt the C names convention */
                           yylval->rvalue.type = VARID;
                           yylval->rvalue.var.name = strndup(yytext,
                                                             ALLOC_NAME_SIZE);
                           /* Default types are int */
                           yylval->rvalue.bit_width = 32;
                           if (yylval->rvalue.var.name == NULL) {
                               fprintf(stderr,
                                       "Error: failed to duplicate var name: "
                                       "\"%s\"\n",
                                       yytext);
                               error_count++;
                               return -1; /* invalid token */
                           }
                           return VAR; }
"fHINTJR(RsV)"           { /* Emit no token */ }
.                        { fprintf(stderr,
                                   "Error: unexpected token \"%s\"\n",
                                   yytext);
                           error_count++;
                           return -1; /* invalid token */
                         }

%%