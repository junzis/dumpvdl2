/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "CMMessageSetVersion1"
 * 	found in "atn-cm.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#ifndef	_CMForwardRequest_H_
#define	_CMForwardRequest_H_


#include <asn_application.h>

/* Including external dependencies */
#include "CMLogonRequest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CMForwardRequest */
typedef CMLogonRequest_t	 CMForwardRequest_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_CMForwardRequest;
asn_struct_free_f CMForwardRequest_free;
asn_struct_print_f CMForwardRequest_print;
asn_constr_check_f CMForwardRequest_constraint;
ber_type_decoder_f CMForwardRequest_decode_ber;
der_type_encoder_f CMForwardRequest_encode_der;
xer_type_decoder_f CMForwardRequest_decode_xer;
xer_type_encoder_f CMForwardRequest_encode_xer;
per_type_decoder_f CMForwardRequest_decode_uper;
per_type_encoder_f CMForwardRequest_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _CMForwardRequest_H_ */
#include <asn_internal.h>
