/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "PMCPDLCMessageSetVersion1"
 * 	found in "atn-cpdlc.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#ifndef	_TimePositionLevel_H_
#define	_TimePositionLevel_H_


#include <asn_application.h>

/* Including external dependencies */
#include "TimePosition.h"
#include "Level.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TimePositionLevel */
typedef struct TimePositionLevel {
	TimePosition_t	 timeposition;
	Level_t	 level;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} TimePositionLevel_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_TimePositionLevel;

#ifdef __cplusplus
}
#endif

#endif	/* _TimePositionLevel_H_ */
#include <asn_internal.h>
