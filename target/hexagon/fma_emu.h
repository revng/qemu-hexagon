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

#ifndef HEXAGON_FMA_EMU_H
#define HEXAGON_FMA_EMU_H

extern int32_t float64_getexp(float64 f64);
extern int32_t float32_getexp(float32 f32);
extern float32 infinite_float32(uint8_t sign);
extern float32 internal_fmafx(float32 a, float32 b, float32 c,
                              int scale, float_status *fp_status);
extern float32 internal_mpyf(float32 a, float32 b,
                             float_status *fp_status);
extern float64 internal_mpyhh(float64 a, float64 b,
                              unsigned long long int accumulated,
                              float_status *fp_status);

#endif