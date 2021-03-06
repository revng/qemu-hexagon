/*
 *  Copyright(c) 2019-2020 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  Check the instruction counters in qemu
 */
#include <stdio.h>
#ifdef __HEXMSGABI_3_SUPPORTED__

#define check(name, val, expect) \
  if (val != expect) { \
    printf("ERROR: " #name " %d != %d\n", val, expect); \
    err++; \
}

int main()
{
  int err = 0;
  int pkt, insn, hvx;

  asm volatile("r2 = #0\n\t"
               "c23 = r2\n\t"
               "c22 = r2\n\t"
               "c21 = r2\n\t"
               "c20 = r2\n\t"
               "r2 = #7\n\t"
               "loop0(1f, #3)\n\t"
               "1:\n\t"
               "    v0.b = vadd(v0.b, v0.b)\n\t"
               "    { p0 = cmp.eq(r2,#5); if (p0.new) jump:nt 2f }\n\t"
               "    {r0 = r1; r1 = r0 }:endloop0\n\t"
               "2:\n\t"
               "%[pkt] = c20\n\t"
               "%[insn] = c21\n\t"
               "%[hvx] = c22\n\t"
               : [pkt] "=r"(pkt), [insn] "=r"(insn), [hvx] "=r"(hvx)
               : : "r0", "r1", "r2", "sa0", "lc0", "v0", "p0");

  check(Packet, pkt, 12);
  check(Instruction, insn, 17);
  check(HVX, hvx, 3);
  puts(err ? "FAIL" : "PASS");
  return err;
}
#else
int main () {puts ("NOT RUN"); return 0;}
#endif
