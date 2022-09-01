// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2019 Intel Corporation. All rights reserved. */

#include <stdio.h>
#include <stdint.h>
#include "iaa_filter.h"

static uint32_t get_element(uint32_t *src1_ptr, uint32_t num_inputs,
			    struct iaa_filter_flags_t *flags_ptr, uint32_t input_idx)
{
	uint64_t qword;
	uint32_t start_bit;
	uint32_t element;
	uint32_t row;

	uint32_t element_width = flags_ptr->src1_width + 1;
	/* For Scan, Extract, Select, RLE Burst and Expand,
	 * drop_high_bits and drop_low_bits will always be 0
	 */
	uint32_t valid_width = element_width -
			       flags_ptr->drop_high_bits -
			       flags_ptr->drop_low_bits;
	uint64_t element_size = ((uint64_t)1) << valid_width;
	uint32_t mask = element_size - 1;

	row = (input_idx * element_width) / 32;

	if (input_idx < (num_inputs - 1))
		qword = (((uint64_t)src1_ptr[row + 1]) << 32) | ((uint64_t)src1_ptr[row]);
	else
		qword = (uint64_t)src1_ptr[row];

	start_bit = (input_idx * element_width) % 32;
	element = ((qword >> start_bit) >> flags_ptr->drop_low_bits) & mask;

	return element;
}

uint32_t iaa_do_scan(void *dst, void *src1, void *src2,
		     uint32_t num_inputs, uint32_t filter_flags)
{
	uint32_t input_idx;
	uint32_t dst_size;
	uint32_t *src1_ptr = (uint32_t *)src1;
	struct iaa_filter_aecs_t *src2_ptr = (struct iaa_filter_aecs_t *)src2;
	uint32_t *dst_ptr = (uint32_t *)dst;
	struct iaa_filter_flags_t *flags_ptr = (struct iaa_filter_flags_t *)(&filter_flags);
	uint32_t element_width = flags_ptr->src1_width + 1;
	uint64_t element_size = ((uint64_t)1) << element_width;
	uint32_t mask = element_size - 1;
	uint32_t element;

	for (input_idx = 0; input_idx < num_inputs; input_idx++) {
		element = get_element(src1_ptr, num_inputs,
				      (struct iaa_filter_flags_t *)&filter_flags, input_idx);
		if (element >= (src2_ptr->low_filter_param & mask) &&
		    element <= (src2_ptr->high_filter_param & mask)) {
			dst_ptr[input_idx / 32] |= 1 << (input_idx % 32);
		}
	}

	if (num_inputs % 8)
		dst_size = num_inputs / 8 + 1;
	else
		dst_size = num_inputs / 8;

	return dst_size;
}
