/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "PMCPDLCMessageSetVersion1"
 * 	found in "atn-cpdlc.asn1"
 * 	`asn1c -fcompound-names -gen-PER`
 */

#ifndef	_LogicalAck_H_
#define	_LogicalAck_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum LogicalAck {
	LogicalAck_required	= 0,
	LogicalAck_notRequired	= 1
} e_LogicalAck;

/* LogicalAck */
typedef long	 LogicalAck_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_LogicalAck;
asn_struct_free_f LogicalAck_free;
asn_struct_print_f LogicalAck_print;
asn_constr_check_f LogicalAck_constraint;
ber_type_decoder_f LogicalAck_decode_ber;
der_type_encoder_f LogicalAck_encode_der;
xer_type_decoder_f LogicalAck_decode_xer;
xer_type_encoder_f LogicalAck_encode_xer;
per_type_decoder_f LogicalAck_decode_uper;
per_type_encoder_f LogicalAck_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _LogicalAck_H_ */
#include <asn_internal.h>
