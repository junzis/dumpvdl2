/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ACSE-1"
 * 	found in "atn-ulcs.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#ifndef	_AE_qualifier_form1_H_
#define	_AE_qualifier_form1_H_


#include <asn_application.h>

/* Including external dependencies */
#include "RelativeDistinguishedName.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AE-qualifier-form1 */
typedef RelativeDistinguishedName_t	 AE_qualifier_form1_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_AE_qualifier_form1;
asn_struct_free_f AE_qualifier_form1_free;
asn_struct_print_f AE_qualifier_form1_print;
asn_constr_check_f AE_qualifier_form1_constraint;
ber_type_decoder_f AE_qualifier_form1_decode_ber;
der_type_encoder_f AE_qualifier_form1_encode_der;
xer_type_decoder_f AE_qualifier_form1_decode_xer;
xer_type_encoder_f AE_qualifier_form1_encode_xer;
per_type_decoder_f AE_qualifier_form1_decode_uper;
per_type_encoder_f AE_qualifier_form1_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _AE_qualifier_form1_H_ */
#include <asn_internal.h>
