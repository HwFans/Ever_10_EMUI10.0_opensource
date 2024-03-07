

#ifndef __OAM_MAIN_H__
#define __OAM_MAIN_H__

/* ����ͷ�ļ����� */
#include "oal_types.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"

#ifdef _PRE_WIFI_DMT
#include "dmt_stub.h"
#endif

/* �궨�� */
#define OAM_BEACON_HDR_LEN 24
#define OAM_TIMER_MAX_LEN  36
#define OAM_PRINT_CRLF_NUM 20 /* ������з��ĸ��� */

#define OAM_FEATURE_NAME_ABBR_LEN 12 /* ����������д��󳤶� */

/* ��ӡ��ʽ���� */
typedef oal_uint32 (*oal_print_func)(const char *pc_string);

/* ������ݸ�ʽ���� */
typedef oal_int8 *oam_va_list;

/* STRUCT���� */
/* ��־�����б��ṹ�嶨�� */
typedef struct {
    oam_feature_enum_uint8 en_feature_id;
    oal_uint8 auc_feature_name_abbr[OAM_FEATURE_NAME_ABBR_LEN]; /* ��0���� */
} oam_software_feature_stru;

/* ÿ��ul_interval������ul_burst����־��� */
typedef enum {
    OAM_RATELIMIT_OUTPUT = 0,
    OAM_RATELIMIT_NOT_OUTPUT,
} oam_ratelimit_output_enum;
typedef oal_uint8 oam_ratelimit_output_enum_uint8;

/* ȫ�ֱ������� */
extern oam_software_feature_stru gst_oam_feature_list_etc[];

/* �������� */
extern oal_int32 oam_main_init_etc(oal_void);
extern oal_void oam_main_exit_etc(oal_void);
extern oal_uint32 oam_send_device_data2sdt_etc(oal_uint8 *pc_string, oal_uint16 len);
extern oal_uint32 oam_set_file_path_etc(oal_int8 *pc_file_path, oal_uint32 ul_length);
extern oal_uint32 oam_set_output_type_etc(oam_output_type_enum_uint8 en_output_type);
extern oal_uint32 oam_print_to_file_etc(const char *pc_string);
extern oal_uint32 oam_print_to_sdt_etc(const char *pc_string);
extern oal_uint32 oam_print_to_console_etc(const char *pc_string);
extern oal_uint32 oam_upload_log_to_sdt_etc(oal_int8 *pc_string);
extern oal_void oam_dump_buff_by_hex_etc(oal_uint8 *puc_buff, oal_int32 l_len, oal_int32 l_num);
extern oam_ratelimit_output_enum_uint8 oam_log_ratelimit_etc(oam_ratelimit_type_enum_uint8 en_ratelimit_type);

#endif /* end of oam_main */