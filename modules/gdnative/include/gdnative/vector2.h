/*************************************************************************/
/*  vector2.h                                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef GODOT_VECTOR2_H
#define GODOT_VECTOR2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gdnative/math_defs.h>

#define GODOT_VECTOR2_SIZE (sizeof(godot_real_t) * 2)

#ifndef GODOT_CORE_API_GODOT_VECTOR2_TYPE_DEFINED
#define GODOT_CORE_API_GODOT_VECTOR2_TYPE_DEFINED
typedef struct {
	uint8_t _dont_touch_that[GODOT_VECTOR2_SIZE];
} godot_vector2;
#endif

#define GODOT_VECTOR2I_SIZE (sizeof(int32_t) * 2)

#ifndef GODOT_CORE_API_GODOT_VECTOR2I_TYPE_DEFINED
#define GODOT_CORE_API_GODOT_VECTOR2I_TYPE_DEFINED
typedef struct {
	uint8_t _dont_touch_that[GODOT_VECTOR2I_SIZE];
} godot_vector2i;
#endif

#include <gdnative/gdnative.h>

void GDAPI godot_vector2_new(godot_vector2 *p_self);
void GDAPI godot_vector2_new_copy(godot_vector2 *r_dest, const godot_vector2 *p_src);
void GDAPI godot_vector2i_new(godot_vector2i *p_self);
void GDAPI godot_vector2i_new_copy(godot_vector2i *r_dest, const godot_vector2i *p_src);
godot_real_t GDAPI *godot_vector2_operator_index(godot_vector2 *p_self, godot_int p_index);
const godot_real_t GDAPI *godot_vector2_operator_index_const(const godot_vector2 *p_self, godot_int p_index);
int32_t GDAPI *godot_vector2i_operator_index(godot_vector2i *p_self, godot_int p_index);
const int32_t GDAPI *godot_vector2i_operator_index_const(const godot_vector2i *p_self, godot_int p_index);

#ifdef __cplusplus
}
#endif

#endif // GODOT_VECTOR2_H
