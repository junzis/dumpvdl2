/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "PMCPDLCMessageSetVersion1"
 * 	found in "atn-cpdlc.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#ifndef	_UnitName_H_
#define	_UnitName_H_


#include <asn_application.h>

/* Including external dependencies */
#include "FacilityDesignation.h"
#include "FacilityName.h"
#include "FacilityFunction.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UnitName */
typedef struct UnitName {
	FacilityDesignation_t	 facilityDesignation;
	FacilityName_t	*facilityName	/* OPTIONAL */;
	FacilityFunction_t	 facilityFunction;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} UnitName_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_UnitName;

#ifdef __cplusplus
}
#endif

#endif	/* _UnitName_H_ */
#include <asn_internal.h>
