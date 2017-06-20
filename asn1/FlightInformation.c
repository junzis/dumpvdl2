/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "PMCPDLCMessageSetVersion1"
 * 	found in "atn-cpdlc.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#include "FlightInformation.h"

static asn_per_constraints_t asn_PER_type_FlightInformation_constr_1 GCC_NOTUSED = {
	{ APC_CONSTRAINED,	 2,  2,  0,  2 }	/* (0..2) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static asn_TYPE_member_t asn_MBR_FlightInformation_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct FlightInformation, choice.routeOfFlight),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_RouteInformation,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"routeOfFlight"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct FlightInformation, choice.levelsOfFlight),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_LevelsOfFlight,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"levelsOfFlight"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct FlightInformation, choice.routeAndLevels),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_RouteAndLevels,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"routeAndLevels"
		},
};
static const asn_TYPE_tag2member_t asn_MAP_FlightInformation_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* routeOfFlight */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* levelsOfFlight */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 } /* routeAndLevels */
};
static asn_CHOICE_specifics_t asn_SPC_FlightInformation_specs_1 = {
	sizeof(struct FlightInformation),
	offsetof(struct FlightInformation, _asn_ctx),
	offsetof(struct FlightInformation, present),
	sizeof(((struct FlightInformation *)0)->present),
	asn_MAP_FlightInformation_tag2el_1,
	3,	/* Count of tags in the map */
	0,
	-1	/* Extensions start */
};
asn_TYPE_descriptor_t asn_DEF_FlightInformation = {
	"FlightInformation",
	"FlightInformation",
	CHOICE_free,
	CHOICE_print,
	CHOICE_constraint,
	CHOICE_decode_ber,
	CHOICE_encode_der,
	CHOICE_decode_xer,
	CHOICE_encode_xer,
	CHOICE_decode_uper,
	CHOICE_encode_uper,
	CHOICE_outmost_tag,
	0,	/* No effective tags (pointer) */
	0,	/* No effective tags (count) */
	0,	/* No tags (pointer) */
	0,	/* No tags (count) */
	&asn_PER_type_FlightInformation_constr_1,
	asn_MBR_FlightInformation_1,
	3,	/* Elements count */
	&asn_SPC_FlightInformation_specs_1	/* Additional specs */
};

