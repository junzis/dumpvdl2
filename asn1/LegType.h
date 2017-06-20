/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "PMCPDLCMessageSetVersion1"
 * 	found in "atn-cpdlc.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#ifndef	_LegType_H_
#define	_LegType_H_


#include <asn_application.h>

/* Including external dependencies */
#include "LegDistance.h"
#include "LegTime.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum LegType_PR {
	LegType_PR_NOTHING,	/* No components present */
	LegType_PR_legDistance,
	LegType_PR_legTime
} LegType_PR;

/* LegType */
typedef struct LegType {
	LegType_PR present;
	union LegType_u {
		LegDistance_t	 legDistance;
		LegTime_t	 legTime;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} LegType_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_LegType;

#ifdef __cplusplus
}
#endif

#endif	/* _LegType_H_ */
#include <asn_internal.h>
