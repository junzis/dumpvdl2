/*
 *  dumpvdl2 - a VDL Mode 2 message decoder and protocol analyzer
 *
 *  Copyright (c) 2017-2019 Tomasz Lemiech <szpajder@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <libacars/libacars.h>		// la_proto_node
#include <libacars/vstring.h>		// la_vstring
#include <libacars/list.h>		// la_list
#include "idrp.h"
#include "dumpvdl2.h"
#include "tlv.h"
#include "atn.h"			// atn_sec_label_parse, atn_sec_label_format_text
#include "x25.h"			// SN_PROTO_CLNP

// Forward declarations
la_type_descriptor const proto_DEF_idrp_pdu;
static tlv_type_descriptor_t tlv_DEF_idrp_attr_no_value;
static tlv_type_descriptor_t tlv_DEF_idrp_ribatt;

static dict const bispdu_types[] = {
	{ .id = BISPDU_TYPE_OPEN,	.val = "Open" },
	{ .id = BISPDU_TYPE_UPDATE,	.val = "Update" },
	{ .id = BISPDU_TYPE_ERROR,	.val = "Error" },
	{ .id = BISPDU_TYPE_KEEPALIVE,	.val = "Keepalive" },
	{ .id = BISPDU_TYPE_CEASE,	.val = "Cease" },
	{ .id = BISPDU_TYPE_RIBREFRESH,	.val = "RIB Refresh" },
	{ .id = 0,			.val = NULL }
};

static dict const open_pdu_errors[] = {
	{ .id = 1, .val = "Unsupported version number" },
	{ .id = 2, .val = "Bad max PDU size" },
	{ .id = 3, .val = "Bad peer RD" },
	{ .id = 4, .val = "Unsupported auth code" },
	{ .id = 5, .val = "Auth failure" },
	{ .id = 6, .val = "Bad RIB-AttsSet" },
	{ .id = 7, .val = "RDC Mismatch" },
	{ .id = 0, .val = NULL }
};

static dict const update_pdu_errors[] = {
	{ .id =  1, .val = "Malformed attribute list" },
	{ .id =  2, .val = "Unrecognized well-known attribute" },
	{ .id =  3, .val = "Missing well-known attribute" },
	{ .id =  4, .val = "Attribute flags error" },
	{ .id =  5, .val = "Attribute length error" },
	{ .id =  6, .val = "RD routing loop" },
	{ .id =  7, .val = "Invalid NEXT_HOP attribute" },
	{ .id =  8, .val = "Optional attribute error" },
	{ .id =  9, .val = "Invalid reachability information" },
	{ .id = 10, .val = "Misconfigured RDCs" },
	{ .id = 11, .val = "Malformed NLRI" },
	{ .id = 12, .val = "Duplicated attributes" },
	{ .id = 13, .val = "Illegal RD path segment" },
	{ .id =  0, .val = NULL }
};

static dict const timer_expired_errors[] = {
	{ .id = 0, .val = "NULL" },
	{ .id = 0, .val = NULL }
};

static dict const FSM_states[] = {
	{ .id = 1, .val = "CLOSED" },
	{ .id = 2, .val = "OPEN-RCVD" },
	{ .id = 3, .val = "OPEN-SENT" },
	{ .id = 4, .val = "CLOSE-WAIT" },
	{ .id = 5, .val = "ESTABLISHED" },
	{ .id = 0, .val = NULL }
};

static dict const RIB_refresh_errors[] = {
	{ .id = 1, .val = "Invalid opcode" },
	{ .id = 2, .val = "Unsupported RIB-Atts" },
	{ .id = 0, .val = NULL }
};

static dict const auth_mechs[] = {
	{ .id = 1, .val = "simple checksum" },
	{ .id = 2, .val = "auth + data integrity check" },
	{ .id = 3, .val = "password" },
	{ .id = 0, .val = NULL }
};

typedef struct {
	char *descr;
	dict *subcodes;
} bispdu_err_t;

static dict const bispdu_errors[] = {
	{
		.id = BISPDU_ERR_OPEN_PDU,
		.val = &(bispdu_err_t){
			"Open PDU error",
			(dict *)&open_pdu_errors
		}
	},
	{
		.id = BISPDU_ERR_UPDATE_PDU,
		.val = &(bispdu_err_t){
			"Update PDU error",
			(dict *)&update_pdu_errors
		}
	},
	{
		.id = BISPDU_ERR_TIMER_EXPIRED,
		.val = &(bispdu_err_t){
			"Hold timer expired",
			(dict *)&timer_expired_errors
		}
	},
	{
		.id = BISPDU_ERR_FSM,
		.val = &(bispdu_err_t){
			"FSM error",
			(dict *)&FSM_states
		}
	},
	{
		.id = BISPDU_ERR_RIB_REFRESH_PDU,
		.val = &(bispdu_err_t){
			"RIB Refresh PDU error",
			(dict *)&RIB_refresh_errors
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

typedef struct {
	uint32_t id;
	uint8_t localpref;
} idrp_route_separator_t;

TLV_PARSER(idrp_route_separator_parse) {
	UNUSED(typecode);
	if(len != 5) {
		debug_print("incorrect length: %zu != 5)", len);
		return NULL;
	}
	NEW(idrp_route_separator_t, ret);
	ret->id = extract_uint32_msbfirst(buf);
	ret->localpref = buf[4];
	return ret;
}

TLV_FORMATTER(idrp_route_separator_format_text) {
	ASSERT(ctx != NULL);
	ASSERT(ctx->vstr != NULL);
	ASSERT(ctx->indent >= 0);

	CAST_PTR(s, idrp_route_separator_t *, data);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	LA_ISPRINTF(ctx->vstr, ctx->indent+1, "ID: %u\n", s->id);
	LA_ISPRINTF(ctx->vstr, ctx->indent+1, "Local preference: %u\n", s->localpref);
}

/*******************
 * RD_PATH
 *******************/

TLV_PARSER(rd_path_segment_parse) {
	UNUSED(typecode);
	la_list *rdi_list = NULL;
	while(len > 1) {
		uint8_t rdi_len = buf[0];
		buf++; len--;
		if(rdi_len == 0) {
			debug_print("RDI length 0 not allowed\n");
			goto fail;
		}
		if(len < rdi_len) {
			debug_print("RDI truncated: remaining: %zu < rdi_len: %u\n",
				len, rdi_len);
			goto fail;
		}
		rdi_list = la_list_append(rdi_list, octet_string_new(buf, len));
		buf += rdi_len; len -= rdi_len;
	}
	return rdi_list;
fail:
	la_list_free(rdi_list);
	return NULL;
}

TLV_FORMATTER(rd_path_segment_format_text) {
	ASSERT(ctx != NULL);
	ASSERT(ctx->vstr != NULL);
	ASSERT(ctx->indent >= 0);

	CAST_PTR(rdi_list, la_list *, data);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	ctx->indent++;
	while(rdi_list != NULL) {
		octet_string_with_ascii_format_text(ctx->vstr, rdi_list->data, ctx->indent);
		EOL(ctx->vstr);
		rdi_list = la_list_next(rdi_list);
	}
	ctx->indent--;
}

TLV_DESTRUCTOR(rd_path_segment_destroy) {
	la_list_free(data);
}

static dict const rd_path_seg_types[] = {
	{
		.id = 1,
		.val = &(tlv_type_descriptor_t){
			.label = "RD_SET",
			.parse = rd_path_segment_parse,
			.format_text = rd_path_segment_format_text,
			.destroy = rd_path_segment_destroy
		}
	},
	{
		.id = 2,
		.val = &(tlv_type_descriptor_t){
			.label = "RD_SEQ",
			.parse = rd_path_segment_parse,
			.format_text = rd_path_segment_format_text,
			.destroy = rd_path_segment_destroy
		}
	},
	{
		.id = 3,
		.val = &(tlv_type_descriptor_t){
			.label = "ENTRY_SEQ",
			.parse = rd_path_segment_parse,
			.format_text = rd_path_segment_format_text,
			.destroy = rd_path_segment_destroy
		}
	},
	{
		.id = 4,
		.val = &(tlv_type_descriptor_t){
			.label = "ENTRY_SET",
			.parse = rd_path_segment_parse,
			.format_text = rd_path_segment_format_text,
			.destroy = rd_path_segment_destroy
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

TLV_PARSER(rd_path_parse) {
	UNUSED(typecode);
	return tlv_parse(buf, len, rd_path_seg_types, 2);
}

TLV_FORMATTER(rd_path_format_text) {
	ASSERT(ctx != NULL);
	ASSERT(ctx->vstr != NULL);
	ASSERT(ctx->indent >= 0);

	CAST_PTR(rd_path, la_list *, data);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "%s:\n", label);
	tlv_list_format_text(ctx->vstr, rd_path, ctx->indent+1);
}

TLV_DESTRUCTOR(rd_path_destroy) {
	tlv_list_destroy(data);
}

static dict const path_attributes[] = {
	{
		.id = 1,
		.val = &(tlv_type_descriptor_t){
			.label = "Route",
			.parse = idrp_route_separator_parse,
			.format_text = idrp_route_separator_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 2,
		.val = &(tlv_type_descriptor_t){
			.label = "Ext. info",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 3,
		.val = &(tlv_type_descriptor_t){
			.label = "RD path",
			.parse = rd_path_parse,
			.format_text = rd_path_format_text,
			.destroy = rd_path_destroy
		}
	},
	{
		.id = 4,
		.val = &(tlv_type_descriptor_t){
			.label = "Next hop",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 5,
		.val = &(tlv_type_descriptor_t){
			.label = "Distribute list inclusions",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 6,
		.val = &(tlv_type_descriptor_t){
			.label = "Distribute list exclusions",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 7,
		.val = &(tlv_type_descriptor_t){
			.label = "Multi exit discriminator",
			.parse = tlv_uint8_parse,
			.format_text = tlv_uint_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 8,
		.val = &(tlv_type_descriptor_t){
			.label = "Transit delay",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 9,
		.val = &(tlv_type_descriptor_t){
			.label = "Residual error",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 10,
		.val = &(tlv_type_descriptor_t){
			.label = "Expense",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 11,
		.val = &(tlv_type_descriptor_t){
			.label = "Locally defined QoS",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 12,
		.val = &(tlv_type_descriptor_t){
			.label = "Hierarchical recording",
			.parse = tlv_octet_string_parse,
			.format_text = tlv_octet_string_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 13,
		.val = &(tlv_type_descriptor_t){
			.label = "RD hop count",
			.parse = tlv_uint8_parse,
			.format_text = tlv_uint_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 14,
		.val = &(tlv_type_descriptor_t){
			.label = "Security",
			.parse = atn_sec_label_parse,
			.format_text = atn_sec_label_format_text,
			.destroy = atn_sec_label_destroy
		}
	},
	{
		.id = 15,
		.val = &(tlv_type_descriptor_t){
			.label = "Capacity",
			.parse = tlv_uint8_parse,
			.format_text = tlv_uint_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 16,
		.val = &(tlv_type_descriptor_t){
			.label = "Priority",
			.parse = tlv_uint8_parse,
			.format_text = tlv_uint_format_text,
			.destroy = NULL
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

TLV_FORMATTER(idrp_attr_no_value_format_text) {
	UNUSED(label);
	ASSERT(ctx != NULL);
	ASSERT(ctx->vstr != NULL);
	ASSERT(ctx->indent >= 0);

// Find the attribute label and print it to indicate its presence in RIB-Att)
	CAST_PTR(typecode, uint8_t *, data);
	CAST_PTR(td, tlv_type_descriptor_t *, dict_search(path_attributes, *typecode));
	if(td != NULL) {
		LA_ISPRINTF(ctx->vstr, ctx->indent, "%s\n", td->label);
	} else {
		LA_ISPRINTF(ctx->vstr, ctx->indent, "Unknown attribute %u\n", *typecode);
	}
}

typedef struct {
	la_list *list;
	int consumed;
} attr_parse_result_t;

static attr_parse_result_t parse_idrp_ribatt(uint8_t *buf, uint32_t len) {
	la_list *attr_list = NULL;
	if(len < 1) {
		goto fail;
	}
	int consumed = 0;
	uint8_t attrs_cnt = buf[0];
	buf++; consumed++; len--;

	uint8_t i = 0;
	while(i < attrs_cnt && len > 0) {
		uint8_t typecode = buf[0];
		buf++; consumed++; len--;
		if(typecode == 11 || typecode == 14) {
// Locally Defined Qos and Security are encoded as full TLVs. Extract the length
// field and run the standard tag parser for them.
			if(len < 2) {
				goto fail;
			}
			size_t tag_len = extract_uint16_msbfirst(buf);
			buf += 2; consumed += 2; len -= 2;
			if(tag_len > len) {
				debug_print("Tag %d: parameter truncated: tag_len=%zu buflen=%u\n",
					typecode, tag_len, len);
				goto fail;
			}
			attr_list = tlv_single_tag_parse(typecode, buf, tag_len,
				path_attributes, attr_list);
			buf += tag_len; consumed += tag_len; len -= tag_len;
		} else {
// Other attributes are encoded as value only.
// Can't store a NULL value in tlv_tag_t, so just store the typecode as value
			NEW(uint8_t, u);
			*u = typecode;
			attr_list = tlv_list_append(attr_list, typecode,
				&tlv_DEF_idrp_attr_no_value, u);
		}
		i++;
	}
	return (attr_parse_result_t){ .list = attr_list, .consumed = consumed };
fail:
	tlv_list_destroy(attr_list);
	return (attr_parse_result_t){ .list = NULL, .consumed = -1 };
}

typedef struct {
	uint8_t num;		// RIBAtt index number
	la_list *attr_list;	// Attributes
} idrp_ribatt_t;

static attr_parse_result_t parse_idrp_ribatts_set(uint8_t *buf, uint32_t len) {
	la_list *ribatt_list = NULL;
	if(len < 1) {
		goto fail;
	}
	int consumed = 0;
	uint8_t ribatts_cnt = buf[0];
	buf++; consumed++; len--;

	uint8_t i = 0;
	while(i < ribatts_cnt && len > 0) {
		attr_parse_result_t result = parse_idrp_ribatt(buf, len);
		debug_print("RibAtt #%u: parse_idrp_ribatt consumed %d octets\n", i, result.consumed);
		if(result.consumed < 0) {
			goto fail;
		}
		buf += result.consumed; consumed += result.consumed; len -= result.consumed;
		NEW(idrp_ribatt_t, ribatt);
		ribatt->num = i;
		ribatt->attr_list = result.list;
		ribatt_list = tlv_list_append(ribatt_list, i, &tlv_DEF_idrp_ribatt, ribatt);
		i++;
	}
	return (attr_parse_result_t){ .list = ribatt_list, .consumed = consumed };
fail:
	tlv_list_destroy(ribatt_list);
	return (attr_parse_result_t){ .list = NULL, .consumed = -1 };
}

TLV_FORMATTER(idrp_ribatt_format_text) {
	UNUSED(label);
	ASSERT(ctx != NULL);
	ASSERT(ctx->vstr != NULL);
	ASSERT(ctx->indent >= 0);

	CAST_PTR(r, idrp_ribatt_t *, data);
	LA_ISPRINTF(ctx->vstr, ctx->indent, "RibAtt #%u:\n", r->num);
	tlv_list_format_text(ctx->vstr, r->attr_list, ctx->indent+1);
}

TLV_DESTRUCTOR(idrp_ribatt_destroy) {
	if(data == NULL) {
		return;
	}
	CAST_PTR(ribatt, idrp_ribatt_t *, data);
	tlv_list_destroy(ribatt->attr_list);
	XFREE(data);
}

// A pseudo-type which only prints its label
static tlv_type_descriptor_t tlv_DEF_idrp_attr_no_value = {
	.label = "",
	.parse = NULL,
	.format_text = idrp_attr_no_value_format_text,
	.destroy = NULL
};

static tlv_type_descriptor_t tlv_DEF_idrp_ribatt = {
	.label = "",
	.parse = NULL,
	.format_text = idrp_ribatt_format_text,
	.destroy = idrp_ribatt_destroy
};

static attr_parse_result_t parse_idrp_confed_ids(uint8_t *buf, uint32_t len) {
	la_list *confed_id_list = NULL;
	if(len < 1) {
		goto fail;
	}
	int consumed = 0;
	uint8_t confed_id_cnt = buf[0];
	buf++; consumed++; len--;

	uint8_t i = 0;
	while(i < confed_id_cnt && len > 0) {
		uint8_t confed_id_len = buf[0];
		buf++; consumed++; len--;
		if(len < confed_id_len) {
			debug_print("Truncated Confed ID #%d: len %u < confed_id_len %u\n",
				i, len, confed_id_len);
			goto fail;
		}
		confed_id_list = la_list_append(confed_id_list, octet_string_new(buf, confed_id_len));
		buf += confed_id_len; consumed += confed_id_len; len -= confed_id_len;
		i++;
	}
	return (attr_parse_result_t){ .list = confed_id_list, .consumed = consumed };
fail:
	la_list_free(confed_id_list);
	return (attr_parse_result_t){ .list = NULL, .consumed = -1 };
}

static int parse_idrp_open_pdu(idrp_pdu_t *pdu, uint8_t *buf, uint32_t len) {
	if(len < 6) {
		debug_print("Truncated Open BISPDU: len %u < 6\n", len);
		return -1;
	}
	if(*buf != BISPDU_OPEN_VERSION) {
		debug_print("Unsupported Open BISPDU version %u\n", *buf);
		return -1;
	}
	int consumed = 0;
	buf++; consumed++; len--;
	pdu->open_holdtime = extract_uint16_msbfirst(buf);
	buf += 2; consumed += 2; len -= 2;
	pdu->open_max_pdu_size = extract_uint16_msbfirst(buf);
	buf += 2; consumed += 2; len -= 2;
	pdu->open_src_rdi.len = *buf++; consumed++; len--;
	if(len < pdu->open_src_rdi.len) {
		debug_print("Truncated source RDI: len %u < rdi_len %zu\n", len, pdu->open_src_rdi.len);
		return -1;
	}
	pdu->open_src_rdi.buf = buf;
	buf += pdu->open_src_rdi.len; consumed += pdu->open_src_rdi.len; len -= pdu->open_src_rdi.len;
	attr_parse_result_t result = parse_idrp_ribatts_set(buf, len);
// FIXME: don't fail catastrophically on unparseable RibAttsSet.
// We can still print most of the dissected packet in this case.
	if(result.consumed < 0) {
		return -1;
	}
	pdu->ribatts_set = result.list;
	buf += result.consumed; consumed += result.consumed; len -= result.consumed;

	result = parse_idrp_confed_ids(buf, len);
	if(result.consumed < 0) {
		debug_print("Failed to parse Confed-IDs\n");
		return -1;
	}
	pdu->confed_ids = result.list;
	buf += result.consumed; consumed += result.consumed; len -= result.consumed;

	if(len < 1) {
		debug_print("PDU truncated before auth mech\n");
		return -1;
	}
	pdu->auth_mech = buf[0];
	buf++; consumed++; len--;
	if(len > 0) {
		debug_print("Auth data: len %u\n", len);
		pdu->auth_data.buf = buf;
		pdu->auth_data.len = len;
		consumed += len;
	}
	return consumed;
}

typedef struct {
	bool is_clnp;
	uint8_t proto_type;
	uint8_t prefix_len;
	octet_string_t prefix;
	octet_string_t proto;
} idrp_nlri_t;

static attr_parse_result_t parse_nlri_list(uint8_t *buf, uint32_t len) {
	la_list *nlri_list = NULL;
	int consumed = 0;

	while(len > 0) {
		if(len < 6) {
			debug_print("NLRI truncated: len %u < 7\n", len);
			goto fail;
		}
		NEW(idrp_nlri_t, nlri);
		nlri_list = la_list_append(nlri_list, nlri);

		nlri->proto_type = buf[0];
		uint8_t proto_len = buf[1];
		buf += 2; consumed += 2; len -= 2;
		if(len < proto_len) {
			debug_print("Protocol field truncated: len %u < proto_len %u\n",
				len, proto_len);
			goto fail;
		}
		nlri->proto.buf = buf;
		nlri->proto.len = proto_len;
		buf += proto_len; consumed += proto_len; len -= proto_len;

		nlri->is_clnp = (nlri->proto_type == 1
			&& nlri->proto.len == 1
			&& nlri->proto.buf[0] == SN_PROTO_CLNP);

		uint16_t addr_len = extract_uint16_msbfirst(buf);
		buf += 2; consumed += 2; len -= 2;
		if(addr_len < 1) {
			debug_print("Addr_length %u too short\n", addr_len);
			goto fail;
		}
		if(len < addr_len) {
			debug_print("Addr_info field truncated: len %u < addr_len %u\n",
				len, addr_len);
			goto fail;
		}
		if(nlri->is_clnp) {
			nlri->prefix_len = buf[0];
			nlri->prefix.buf = buf + 1;
			nlri->prefix.len = addr_len - 1;
		} else {
			nlri->prefix.buf = buf;
			nlri->prefix.len = addr_len;
		}
		buf += addr_len; consumed += addr_len; len -= addr_len;
	}
	return (attr_parse_result_t){ .list = nlri_list, .consumed = consumed };
fail:
	la_list_free(nlri_list);
	return (attr_parse_result_t){ .list = NULL, .consumed = -1 };
}

static int parse_idrp_update_pdu(idrp_pdu_t *pdu, uint8_t *buf, uint32_t len) {
	if(len < 4) {
		debug_print("Truncated Update BISPDU: len %u < 4\n", len);
		return -1;
	}
	int consumed = 0;
	uint16_t num_withdrawn = extract_uint16_msbfirst(buf);
	buf += 2; consumed += 2; len -= 2;
	if(num_withdrawn > 0) {
		if(len < num_withdrawn * 4) {
			debug_print("Withdrawn Routes field truncated: len %u < expected %u\n", len, 4 * num_withdrawn);
			return -1;
		}
		for(uint16_t i = 0; i < num_withdrawn; i++) {
			NEW(uint32_t, u);
			*u = extract_uint32_msbfirst(buf);
			pdu->withdrawn_routes = la_list_append(pdu->withdrawn_routes, u);
			buf += 4; consumed += 4; len -= 4;
		}
	}
	if(len < 2) {
		debug_print("BISPDU truncated after withdrawn routes: len %u < 2\n", len);
		return -1;
	}
	uint16_t total_attrib_len = extract_uint16_msbfirst(buf);
	buf += 2; consumed += 2; len -= 2;
	if(total_attrib_len > 0) {
		if(len < total_attrib_len) {
			debug_print("Path attributes field truncated: len %u < expected %u\n", len, total_attrib_len);
			return -1;
		}
		while(total_attrib_len > 4) {				// flag + typecode + length
// buf[0] is flag - not too interesting, so skip it
			uint8_t typecode = buf[1];
			buf += 2; consumed += 2; len -= 2; total_attrib_len -= 2;
			uint16_t alen = extract_uint16_msbfirst(buf);
			buf += 2; consumed += 2; len -= 2; total_attrib_len -= 2;
			if(len < alen) {
				debug_print("Attribute value truncated: len %u < expected %u\n", len, alen);
				return -1;
			}
			pdu->path_attributes = tlv_single_tag_parse(typecode, buf, alen,
				path_attributes, pdu->path_attributes);
			buf += alen; consumed += alen; len -= alen; total_attrib_len -= alen;
		}
		if(total_attrib_len > 0) {
			debug_print("total_attrib_len disagrees with length of the attributes: (%u octets left)\n",
				total_attrib_len);
			return -1;
		}
	}
	attr_parse_result_t result = parse_nlri_list(buf, len);
	if(result.consumed >= 0) {
		pdu->nlri_list = result.list;	// may be NULL for empty NLRI list - this is harmless
		consumed += result.consumed;
	} else {
// NLRI parser failed - store NLRI field as octet_string to be printed in raw
		pdu->data = octet_string_new(buf, len);
		consumed += len;
	}
	return consumed;
}

static int parse_idrp_error_pdu(idrp_pdu_t *pdu, uint8_t *buf, uint32_t len) {
	if(len < 2) {
		debug_print("Truncated Error BISPDU: len %u < 2\n", len);
		return -1;
	}
	int consumed = 0;
	uint8_t err_code = *buf++;
	uint8_t err_subcode = *buf++;
	consumed += 2; len -= 2;

	debug_print("code=%u subcode=%u\n", err_code, err_subcode);
	if(err_code == BISPDU_ERR_FSM) {
// upper nibble of subcode contains BISPDU type which this error PDU is related to.
// lower nibble contains current FSM state
		pdu->err_fsm_bispdu_type = err_subcode >> 4;
		pdu->err_fsm_state = err_subcode & 0xf;
	}
	pdu->err_code = err_code;
	pdu->err_subcode = err_subcode;
	pdu->data = octet_string_new(buf, len);
	consumed += len;
	return consumed;
}

la_proto_node *idrp_pdu_parse(uint8_t *buf, uint32_t len, uint32_t *msg_type) {
	NEW(idrp_pdu_t, pdu);
	la_proto_node *node = la_proto_node_new();
	node->td = &proto_DEF_idrp_pdu;
	node->data = pdu;
	node->next = NULL;

	pdu->err = true;	// fail-safe default
	if(len < BISPDU_HDR_LEN) {
		debug_print("Too short (len %u < min len %u)\n", len, BISPDU_HDR_LEN);
		goto end;
	}
	uint8_t *ptr = buf;
	uint32_t remaining = len;
	CAST_PTR(hdr, idrp_hdr_t *, ptr);
	uint16_t pdu_len = extract_uint16_msbfirst(hdr->len);
	debug_print("pid: %02x len: %u type: %u seq: %u ack: %u coff: %u cavail: %u\n",
		hdr->pid, pdu_len, hdr->type, ntohl(hdr->seq), ntohl(hdr->ack), hdr->coff, hdr->cavail);
	debug_print_buf_hex(hdr->validation, 16, "Validation:\n");
	if(remaining < pdu_len) {
		debug_print("Too short (len %u < PDU len %u)\n", remaining, pdu_len);
		goto end;
	}
	ptr += BISPDU_HDR_LEN; remaining -= BISPDU_HDR_LEN; pdu_len -= BISPDU_HDR_LEN;
	debug_print("skipping %u hdr octets, %u octets remaining\n", BISPDU_HDR_LEN, remaining);
	int result = 0;
	switch(hdr->type) {
	case BISPDU_TYPE_OPEN:
		result = parse_idrp_open_pdu(pdu, ptr, pdu_len);
		break;
	case BISPDU_TYPE_UPDATE:
		result = parse_idrp_update_pdu(pdu, ptr, pdu_len);
		break;
	case BISPDU_TYPE_ERROR:
		result = parse_idrp_error_pdu(pdu, ptr, pdu_len);
		break;
	case BISPDU_TYPE_KEEPALIVE:
	case BISPDU_TYPE_CEASE:
		break;
	case BISPDU_TYPE_RIBREFRESH:
		break;
	default:
		debug_print("Unknown BISPDU type 0x%02x\n", hdr->type);
		result = -1;
	}
	if(result < 0) {		// unparseable PDU
		goto end;
	}
	ptr += result; remaining -= result;
	if(remaining > 0) {
		node->next= unknown_proto_pdu_new(ptr, remaining);
	}

	if(hdr->type == BISPDU_TYPE_KEEPALIVE) {
		*msg_type |= MSGFLT_IDRP_KEEPALIVE;
	} else {
		*msg_type |= MSGFLT_IDRP_NO_KEEPALIVE;
	}

	pdu->hdr = hdr;
	pdu->err = false;
	return node;
end:
	node->next = unknown_proto_pdu_new(buf, len);
	return node;
}

static void idrp_error_format_text(la_vstring *vstr, idrp_pdu_t *pdu, int indent) {
	ASSERT(vstr != NULL);
	ASSERT(pdu != NULL);
	ASSERT(indent >= 0);

	CAST_PTR(err, bispdu_err_t *, dict_search(bispdu_errors, pdu->err_code));
	LA_ISPRINTF(vstr, indent, "Code: %u (%s)\n", pdu->err_code, err ? err->descr : "unknown");
	if(!err) {
		LA_ISPRINTF(vstr, indent, "Subcode: %u (unknown)\n", pdu->err_subcode);
		goto print_err_payload;
	}
	if(pdu->err_code == BISPDU_ERR_FSM) {	// special case
		CAST_PTR(bispdu_name, char *, dict_search(bispdu_types, pdu->err_fsm_bispdu_type));
		CAST_PTR(fsm_state_name, char *, dict_search(FSM_states, pdu->err_fsm_state));
		LA_ISPRINTF(vstr, indent, "Erroneous BISPDU type: %s\n",
			bispdu_name ? bispdu_name : "unknown");
		LA_ISPRINTF(vstr, indent, "FSM state: %s\n",
			fsm_state_name ? fsm_state_name : "unknown");
	} else {
		CAST_PTR(subcode, char *, dict_search(err->subcodes, pdu->err_subcode));
		LA_ISPRINTF(vstr, indent, "Subcode: %u (%s)\n", pdu->err_subcode, subcode ? subcode : "unknown");
	}
print_err_payload:
	if(pdu->data != NULL && pdu->data->buf != NULL && pdu->data->len > 0) {
		LA_ISPRINTF(vstr, indent, "%s: ", "Error data");
		octet_string_format_text(vstr, pdu->data, 0);
		EOL(vstr);
	}
}

void idrp_pdu_format_text(la_vstring * const vstr, void const * const data, int indent) {
	ASSERT(vstr != NULL);
	ASSERT(data);
	ASSERT(indent >= 0);

	CAST_PTR(pdu, idrp_pdu_t *, data);
	if(pdu->err == true) {
		LA_ISPRINTF(vstr, indent, "%s", "-- Unparseable IDRP PDU\n");
		return;
	}
	idrp_hdr_t *hdr = pdu->hdr;
	CAST_PTR(bispdu_name, char *, dict_search(bispdu_types, hdr->type));
	LA_ISPRINTF(vstr, indent, "IDRP %s: seq: %u ack: %u credit_offered: %u credit_avail: %u\n",
		bispdu_name, ntohl(hdr->seq), ntohl(hdr->ack), hdr->coff, hdr->cavail);
	indent++;
	switch(pdu->hdr->type) {
	case BISPDU_TYPE_OPEN:
		LA_ISPRINTF(vstr, indent, "Hold Time: %u seconds\n", pdu->open_holdtime);
		LA_ISPRINTF(vstr, indent, "Max. PDU size: %u octets\n", pdu->open_max_pdu_size);
		{
			LA_ISPRINTF(vstr, indent, "%s: ", "Source RDI");
			octet_string_with_ascii_format_text(vstr, &pdu->open_src_rdi, 0);
			EOL(vstr);
			LA_ISPRINTF(vstr, indent, "%s:\n", "RIB Attribute Set");
			if(pdu->ribatts_set != NULL) {
				tlv_list_format_text(vstr, pdu->ribatts_set, indent+1);
			}
			if(pdu->confed_ids != NULL) {
				LA_ISPRINTF(vstr, indent, "%s:\n", "Confederation IDs");
				indent++;
				for(la_list *p = pdu->confed_ids; p != NULL; p = p->next) {
					octet_string_with_ascii_format_text(vstr, p->data, indent);
					EOL(vstr);
				}
				indent--;
			}
			CAST_PTR(auth_mech_name, char *, dict_search(auth_mechs, pdu->auth_mech));
			LA_ISPRINTF(vstr, indent, "Auth mechanism: %s\n",
				auth_mech_name ? auth_mech_name : "unknown");
			if(pdu->auth_data.buf != NULL && pdu->auth_data.len > 0) {
				LA_ISPRINTF(vstr, indent, "%s: ", "Auth data");
				octet_string_format_text(vstr, &pdu->auth_data, 0);
				EOL(vstr);
			}
		}
		break;
	case BISPDU_TYPE_UPDATE:
		if(pdu->withdrawn_routes != NULL) {
			LA_ISPRINTF(vstr, indent, "%s", "Withdrawn Routes:\n");
			indent++;
			for(la_list *p = pdu->withdrawn_routes; p != NULL; p = p->next) {
				LA_ISPRINTF(vstr, indent, "ID: %lu\n", *(uint32_t *)(p->data));
			}
			indent--;
		}
		if(pdu->path_attributes != NULL) {
			tlv_list_format_text(vstr, pdu->path_attributes, indent);
		}

		if(pdu->nlri_list != NULL) {
			la_list *n = pdu->nlri_list;
			while(n != NULL) {
				LA_ISPRINTF(vstr, indent, "%s:\n", "Reachability info");
				indent++;
				CAST_PTR(dest, idrp_nlri_t *, n->data);
				if(dest->is_clnp) {
					LA_ISPRINTF(vstr, indent, "%s\n", "Protocol: CLNP");
					LA_ISPRINTF(vstr, indent, "Prefix length: %u\n", dest->prefix_len);
				} else {
					LA_ISPRINTF(vstr, indent, "%s", "Protocol: ");
					octet_string_format_text(vstr, &dest->proto, 0);
					EOL(vstr);
				}
				LA_ISPRINTF(vstr, indent, "%s: ", "Dest. address prefix");
				octet_string_with_ascii_format_text(vstr, &dest->prefix, 0);
				EOL(vstr);
				indent--;
				n = la_list_next(n);
			}
		} else if(pdu->data != NULL && pdu->data->buf != NULL && pdu->data->len > 0) {
			LA_ISPRINTF(vstr, indent, "%s\n", "-- Unparseable NLRI");
			octet_string_with_ascii_format_text(vstr, pdu->data, indent+1);
			EOL(vstr);
		}
		break;
	case BISPDU_TYPE_ERROR:
		idrp_error_format_text(vstr, pdu, indent);
		break;
	case BISPDU_TYPE_KEEPALIVE:
	case BISPDU_TYPE_CEASE:
		break;
	case BISPDU_TYPE_RIBREFRESH:
		break;
	}
}

void idrp_pdu_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	CAST_PTR(pdu, idrp_pdu_t *, data);
	la_list_free(pdu->withdrawn_routes);
	tlv_list_destroy(pdu->path_attributes);
	tlv_list_destroy(pdu->ribatts_set);
	la_list_free(pdu->confed_ids);
	la_list_free(pdu->nlri_list);
	XFREE(pdu->data);
	XFREE(data);
}

la_type_descriptor const proto_DEF_idrp_pdu = {
	.format_text = idrp_pdu_format_text,
	.destroy = idrp_pdu_destroy
};
