

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/* 1 头文件包含 */
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "hmac_dfx.h"
#include "hmac_ext_if.h"
#include "plat_cali.h"
#include "hmac_device.h"
#include "hmac_resource.h"
#include "hmac_config.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DFX_C

/* 2 全局变量定义 */
#ifdef _PRE_WLAN_FEATURE_DFR
hmac_dfr_info_stru g_st_dfr_info; /* DFR异常复位开关 */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_mutex_stru     g_st_dfr_mutex; /* dfr使用的互斥锁 */
#endif
#endif

OAL_STATIC oam_cfg_data_stru g_ast_cfg_data[OAM_CFG_TYPE_BUTT] = {
    { OAM_CFG_TYPE_MAX_ASOC_USER, "USER_SPEC", "max_asoc_user", 31 },
};

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
oal_uint16 wlan_assoc_user_max_num = 32; /* 关联用户的最大个数 Root AP模式下为32个,Repeater模式下是15个 */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* 由于WL_L2_DRAM大小限制，目前暂时开放2个业务vap，整体规格开放待后续优化 TBD */
oal_uint32 g_ul_wlan_vap_max_num_per_device = 4 + 1; /* 4个AP + 1个配置vap */
#else
oal_uint32 g_ul_wlan_vap_max_num_per_device = 4 + 1; /* 4个AP + 1个配置vap */
#endif

#else
extern oal_uint16 wlan_assoc_user_max_num;
extern oal_uint32 g_ul_wlan_vap_max_num_per_device;
#endif
#define CHR_QUERY_WIFI_INFO_TIME  (5 * OAL_TIME_HZ)

/* 3 函数实现  TBD 置换函数名称 */

oal_int32 oam_cfg_get_item_by_id(oam_cfg_type_enum_uint16 en_cfg_type)
{
    oal_uint32 ul_loop;

    for (ul_loop = 0; ul_loop < OAM_CFG_TYPE_BUTT; ul_loop++) {
        if (en_cfg_type == g_ast_cfg_data[ul_loop].en_cfg_type) {
            break;
        }
    }

    if (OAM_CFG_TYPE_BUTT == ul_loop) {
        OAL_IO_PRINT("oam_cfg_get_item_by_id::get cfg item failed!\n");
        return -OAL_FAIL;
    }

    return g_ast_cfg_data[ul_loop].l_val;
}

#if ((defined(_PRE_PRODUCT_ID_HI110X_DEV)) || (defined(_PRE_PRODUCT_ID_HI110X_HOST)))

oal_uint32 hmac_custom_init(oal_uint32 ul_psta_enable)
{
    /* 硬件限制:3个STA; 2个AP */
    /* 软件规格:
     * 1)AP 模式:  2个ap + 1个配置vap
     * 2)STA 模式: 3个sta + 1个配置vap
     * 3)STA+P2P共存模式:  1个sta + 1个P2P_dev + 1个P2P_GO/Client + 1个配置vap
     * 4)STA+Proxy STA共存模式:  1个sta + ?个proxy STA + 1个配置vap
     */
    wlan_assoc_user_max_num = 8;
    // TBD 设定为WLAN_ASSOC_USER_MAX_NUM_LIMIT;
    g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;

    return OAL_SUCC;
}

#else

oal_uint32 hmac_custom_init(oal_uint32 ul_psta_enable)
{

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (ul_psta_enable) {
        wlan_assoc_user_max_num = 15;
        g_ul_wlan_vap_max_num_per_device = WLAN_REPEATER_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE; /* 1个AP, 1个sta，15个Proxy STA，1个配置vap */
    } else
#endif
    {
        wlan_assoc_user_max_num = WLAN_ASSOC_USER_MAX_NUM_LIMIT;
        g_ul_wlan_vap_max_num_per_device = WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE + WLAN_CONFIG_VAP_MAX_NUM_PER_DEVICE;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 oam_cfg_restore_all_item(oal_int32 al_default_cfg_data[])
{
    oal_uint32 ul_loop;

    for (ul_loop = 0; ul_loop < OAM_CFG_TYPE_BUTT; ul_loop++) {
        g_ast_cfg_data[ul_loop].l_val = al_default_cfg_data[ul_loop];
    }

    return OAL_SUCC;
}


oal_int32 oam_cfg_get_all_item(oal_void)
{
    oal_int32     al_default_cfg_data[OAM_CFG_TYPE_BUTT] = {0};
    oal_uint8    *puc_plaintext;
    oal_uint8    *puc_ciphertext;
    oal_uint32    ul_file_size = 0;
    oal_uint32    ul_loop;
    oal_int32     l_ret;
    oal_int       i_key[OAL_AES_KEYSIZE_256] = { 0x1,0x2,0x3,0x4d,0x56,0x10,0x11,0x12,
                                       		    0x1,0x2,0x3,0x4d,0x56,0x10,0x11,0x12,
                                       		    0x1,0x2,0x3,0x4d,0x56,0x10,0x11,0x12,
                                       		    0x1,0x2,0x3,0x4d,0x56,0x10,0x11,0x12
											   };

    oal_aes_key_stru st_aes_key;

    /* 保存默认配置，如果获取配置文件中信息的时候中间有失败的情况，则需要恢复
       前面全局配置信息，其它模块加载的时候可以按照默认配置加载
    */
    for (ul_loop = 0; ul_loop < OAM_CFG_TYPE_BUTT; ul_loop++) {
        al_default_cfg_data[ul_loop] = g_ast_cfg_data[ul_loop].l_val;
    }

    /* 获取文件大小并获取文件指针 */
    l_ret = oal_file_size(&ul_file_size);
    if (OAL_SUCC != l_ret) {
        OAL_IO_PRINT("oam_cfg_get_all_item::get file size failed!\n");
        return l_ret;
    }

    /* 将配置文件中的所有数据读到一个缓冲区里，此时数据是加密的 */
    puc_ciphertext = oal_memalloc(ul_file_size + OAM_CFG_STR_END_SIGN_LEN);
    if (OAL_PTR_NULL == puc_ciphertext) {
        OAL_IO_PRINT("oam_cfg_get_all_item::alloc ciphertext buf failed! load ko with default cfg!\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(puc_ciphertext, ul_file_size + OAM_CFG_STR_END_SIGN_LEN);

    l_ret = oam_cfg_read_file_to_buf((oal_int8 *)puc_ciphertext, ul_file_size);
    if (OAL_SUCC != l_ret) {
        OAL_IO_PRINT("oam_cfg_get_all_item::get cfg data from file failed! fail id-->%d\n", l_ret);
        oal_free(puc_ciphertext);
        return l_ret;
    }

    /* 申请明文空间，并将密文解密 */
    puc_plaintext = oal_memalloc(ul_file_size + OAM_CFG_STR_END_SIGN_LEN);
    if (OAL_PTR_NULL == puc_plaintext) {
        OAL_IO_PRINT("oam_cfg_get_all_item::alloc pc_plaintext buf failed! load ko with default cfg!\n");
        oal_free(puc_ciphertext);

        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(puc_plaintext, ul_file_size + OAM_CFG_STR_END_SIGN_LEN);

    /* 解密 */
    l_ret = (oal_int32)oal_aes_expand_key(&st_aes_key, (oal_uint8 *)i_key, OAL_AES_KEYSIZE_256);
    if (OAL_SUCC != l_ret) {
        oal_free(puc_plaintext);
        oal_free(puc_ciphertext);

        return l_ret;
    }

    oam_cfg_decrypt_all_item(&st_aes_key, (oal_int8 *)puc_ciphertext,
                             (oal_int8 *)puc_plaintext, ul_file_size);

    /* 获取配置文件中每一项的信息，保存到OAM内部结构中 */
    for (ul_loop = 0; ul_loop < OAM_CFG_TYPE_BUTT; ul_loop++) {
        l_ret = oam_cfg_get_one_item((oal_int8 *)puc_plaintext,
                                     g_ast_cfg_data[ul_loop].pc_section,
                                     g_ast_cfg_data[ul_loop].pc_key,
                                     &g_ast_cfg_data[ul_loop].l_val);

        /* 如果获取某一配置值不成功，则恢复配置项的默认值 */
        if (OAL_SUCC != l_ret) {
            OAL_IO_PRINT("oam_cfg_get_all_item::get cfg item fail! ul_loop=%d\n", ul_loop);

            oam_cfg_restore_all_item(al_default_cfg_data);
            oal_free(puc_plaintext);
            oal_free(puc_ciphertext);

            return l_ret;
        }
    }

    /* 释放缓冲区 */
    oal_free(puc_plaintext);
    oal_free(puc_ciphertext);

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_1102A_CHR

/**********************全局变量****************************/
/* 去关联共有7种原因(0~6),默认值设置为7，表示没有去关联触发 */
hmac_chr_disasoc_reason_stru g_hmac_chr_disasoc_reason = { 0, DMAC_DISASOC_MISC_BUTT };

/* 关联字码, 新增4种私有定义字码5200-5203 */
oal_uint16 g_hmac_chr_connect_code = 0;

hmac_chr_del_ba_info_stru g_hmac_chr_del_ba_info = { 0, 0, MAC_UNSPEC_REASON };

/**********************获取全局变量地址****************************/
hmac_chr_disasoc_reason_stru *hmac_chr_disasoc_reason_get_pointer(void)
{
    return &g_hmac_chr_disasoc_reason;
}

oal_uint16 *hmac_chr_connect_code_get_pointer(void)
{
    return &g_hmac_chr_connect_code;
}

hmac_chr_del_ba_info_stru *hmac_chr_del_ba_info_get_pointer(void)
{
    return &g_hmac_chr_del_ba_info;
}

/* 回复全部变量的初始值 */
oal_void hmac_chr_info_clean(void)
{
    g_hmac_chr_disasoc_reason.us_user_id = 0;
    g_hmac_chr_disasoc_reason.en_disasoc_reason = DMAC_DISASOC_MISC_BUTT;
    g_hmac_chr_connect_code = 0;
    g_hmac_chr_del_ba_info.uc_ba_num = 0;
    g_hmac_chr_del_ba_info.uc_del_ba_tid = 0;
    g_hmac_chr_del_ba_info.en_del_ba_reason = MAC_UNSPEC_REASON;

    return;
}

/**********************CHR打点和获取****************************/
/* 现阶段CHR只考虑STA状态(不考虑P2P)，所以不区分vap_id */
/* 打点 */
oal_void hmac_chr_set_disasoc_reason(oal_uint16 user_id, oal_uint16 reason_id)
{
    hmac_chr_disasoc_reason_stru *pst_disasoc_reason = OAL_PTR_NULL;

    pst_disasoc_reason = hmac_chr_disasoc_reason_get_pointer();

    pst_disasoc_reason->us_user_id = user_id;
    pst_disasoc_reason->en_disasoc_reason = (dmac_disasoc_misc_reason_enum)reason_id;

    return;
}
/* 获取 */
oal_void hmac_chr_get_disasoc_reason(hmac_chr_disasoc_reason_stru *pst_disasoc_reason)
{
    hmac_chr_disasoc_reason_stru *pst_disasoc_reason_temp = OAL_PTR_NULL;

    pst_disasoc_reason_temp = hmac_chr_disasoc_reason_get_pointer();

    pst_disasoc_reason->us_user_id = pst_disasoc_reason_temp->us_user_id;
    pst_disasoc_reason->en_disasoc_reason = pst_disasoc_reason_temp->en_disasoc_reason;

    return;
}

/* 打点 */
/* 梳理删减聚合的流程 计数统计 */
oal_void hmac_chr_set_del_ba_info(oal_uint8 uc_tid, oal_uint16 reason_id)
{
    hmac_chr_del_ba_info_stru *pst_del_ba_info = OAL_PTR_NULL;

    pst_del_ba_info = hmac_chr_del_ba_info_get_pointer();

    pst_del_ba_info->uc_del_ba_tid = uc_tid;
    pst_del_ba_info->en_del_ba_reason = (mac_reason_code_enum)reason_id;

    return;
}

/* 获取 */
oal_void hmac_chr_get_del_ba_info(mac_vap_stru *pst_mac_vap, hmac_chr_del_ba_info_stru *pst_del_ba_reason)
{
    hmac_chr_del_ba_info_stru *pst_del_ba_info = OAL_PTR_NULL;

    pst_del_ba_info = hmac_chr_del_ba_info_get_pointer();

    pst_del_ba_reason->uc_ba_num = 0;
    pst_del_ba_reason->uc_del_ba_tid = pst_del_ba_info->uc_del_ba_tid;
    pst_del_ba_reason->en_del_ba_reason = pst_del_ba_info->en_del_ba_reason;

    return;
}
oal_void hmac_chr_set_connect_code(oal_uint16 connect_code)
{
    oal_uint16 *pus_connect_code = OAL_PTR_NULL;

    pus_connect_code = hmac_chr_connect_code_get_pointer();
    *pus_connect_code = connect_code;
    return;
}

oal_void hmac_chr_get_connect_code(oal_uint16 *pus_connect_code)
{
    pus_connect_code = hmac_chr_connect_code_get_pointer();
    return;
}


OAL_STATIC oal_void hmac_chr_wifi_ext_info_completion(hmac_vap_stru         *pst_hmac_vap,
                                                              chr_wifi_ext_info_stru *pst_wifi_ext_info)
{
    oal_uint8 uc_idx;
    chr_wifi_ext_info_query_stru st_chr_wifi_ext_info_query = pst_hmac_vap->st_chr_wifi_ext_info_query;

    pst_wifi_ext_info->ul_tbtt_cnt = st_chr_wifi_ext_info_query.st_chr_wifi_ext_info.ul_tbtt_cnt;
    pst_wifi_ext_info->ul_rx_beacon_cnt = st_chr_wifi_ext_info_query.st_chr_wifi_ext_info.ul_rx_beacon_cnt;

    pst_wifi_ext_info->en_distance_id = st_chr_wifi_ext_info_query.st_chr_wifi_ext_info.en_distance_id;
    pst_wifi_ext_info->ul_upc1_01_data = st_chr_wifi_ext_info_query.st_chr_wifi_ext_info.ul_upc1_01_data;

    for (uc_idx = 0; uc_idx < WLAN_TID_MAX_NUM; uc_idx++) {
        pst_wifi_ext_info->auc_is_paused[uc_idx] = st_chr_wifi_ext_info_query.st_chr_wifi_ext_info.auc_is_paused[uc_idx];
    }
}


OAL_STATIC oal_void hmac_chr_get_wifi_ext_info_from_device(mac_vap_stru *pst_mac_vap,
                                                                    chr_wifi_ext_info_stru *pst_wifi_ext_info,
                                                                    oal_uint32 chr_event_id)
{
    mac_user_stru   *pst_mac_user;
    hmac_vap_stru   *pst_hmac_vap;
    oal_int         i_leftime;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL){
        return;
    }

    /* 如果没有关联上AP则不下发查询，防止空等 */
    pst_mac_user = mac_res_get_mac_user(pst_mac_vap->uc_assoc_vap_id);
    if (pst_mac_user == OAL_PTR_NULL) {
        /* 如果是异常断线导致的没有user，则通过get station接口获取beacon和tbtt计数 */
        if (chr_event_id == CHR_WIFI_DISCONNECT_QUERY_EVENTID) {
            pst_wifi_ext_info->ul_rx_beacon_cnt = pst_hmac_vap->st_station_info_extend.ul_bcn_cnt;
            pst_wifi_ext_info->ul_tbtt_cnt = pst_hmac_vap->st_station_info_extend.ul_tbtt_cnt;
         }
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                    "{hmac_chr_get_wifi_ext_info_from_dmac::pst_mac_user is null,record beacon [%d],tbtt_cnt[%d].}",
                    pst_wifi_ext_info->ul_rx_beacon_cnt, pst_wifi_ext_info->ul_tbtt_cnt);
        return;
    }

    pst_hmac_vap->st_chr_wifi_ext_info_query.en_chr_info_ext_query_completed_flag = OAL_FALSE;

    hmac_config_query_chr_info_ext(pst_mac_vap, 0, 0);

    i_leftime = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->st_chr_wifi_ext_info_query.st_query_chr_wait_q,
                (pst_hmac_vap->st_chr_wifi_ext_info_query.en_chr_info_ext_query_completed_flag == OAL_TRUE),
                CHR_QUERY_WIFI_INFO_TIME);
    if (i_leftime == 0) {
        /* 超时还没有上报扫描结束 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                        "{hmac_chr_get_wifi_ext_info_from_dmac::query info wait for %ld ms timeout!}",
                         CHR_QUERY_WIFI_INFO_TIME);
        return;
    } else if (i_leftime < 0) {
        /* 定时器内部错误 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                        "{hmac_chr_get_wifi_ext_info_from_dmac::query info wait for %ld ms error!}",
                         CHR_QUERY_WIFI_INFO_TIME);
        return;
    } else {
        hmac_chr_wifi_ext_info_completion(pst_hmac_vap, pst_wifi_ext_info);
        return;
    }

}

oal_void hmac_chr_get_vap_info(mac_vap_stru *pst_mac_vap, hmac_chr_vap_info_stru *pst_chr_vap_info)
{
    mac_user_stru     *pst_mac_user;
    mac_device_stru   *pst_mac_device;
    hmac_vap_stru     *pst_hmac_vap;

    pst_mac_device = mac_res_get_dev(0);

    pst_chr_vap_info->uc_vap_state = pst_mac_vap->en_vap_state;
    pst_chr_vap_info->uc_vap_num = pst_mac_device->uc_vap_num;
    pst_chr_vap_info->uc_vap_rx_nss = pst_mac_vap->en_vap_rx_nss;
    pst_chr_vap_info->uc_protocol = pst_mac_vap->en_protocol;

    /* sta 关联的AP的能力 */
    pst_mac_user = mac_res_get_mac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_PTR_NULL != pst_mac_user) {
        pst_chr_vap_info->uc_ap_spatial_stream_num = pst_mac_user->uc_num_spatial_stream;
        pst_chr_vap_info->bit_ap_11ntxbf = pst_mac_user->st_cap_info.bit_11ntxbf;
        pst_chr_vap_info->bit_ap_qos = pst_mac_user->st_cap_info.bit_qos;
        pst_chr_vap_info->uc_ap_protocol_mode = pst_mac_user->en_protocol_mode;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    pst_chr_vap_info->bit_sta_11ntxbf = pst_mac_vap->st_cap_flag.bit_11ntxbf;
    pst_chr_vap_info->bit_is_dbac_running = mac_is_dbac_running(pst_mac_device);

    if(pst_hmac_vap != OAL_PTR_NULL)
    {
        pst_chr_vap_info->bit_ampdu_active    = pst_hmac_vap->en_ampdu_tx_on_switch;
        pst_chr_vap_info->bit_amsdu_active    = pst_hmac_vap->en_amsdu_ampdu_active;
    }
    pst_chr_vap_info->bit_is_dbdc_running = 0;
    pst_chr_vap_info->bit_ap_1024qam_cap = 0;
    return;
}

oal_uint32 hmac_chr_get_chip_info(oal_uint32 chr_event_id)
{
    oal_uint8                uc_vap_index;
    mac_vap_stru             *pst_mac_vap = OAL_PTR_NULL;
    mac_device_stru          *pst_mac_device = OAL_PTR_NULL;
    hmac_chr_info            *pst_hmac_chr_info = OAL_PTR_NULL;
    oal_netbuf_stru          *pst_buf = OAL_PTR_NULL;

    pst_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, HMAC_CHR_NETBUF_ALLOC_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_buf)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* OAL_SIZEOF(dmac_chr_info) 实测大小252 */
    oal_netbuf_put(pst_buf, OAL_SIZEOF(hmac_chr_info));
    pst_hmac_chr_info = (hmac_chr_info *)OAL_NETBUF_DATA(pst_buf);
    oal_memset(pst_hmac_chr_info, 0, OAL_SIZEOF(hmac_chr_info));

    pst_mac_device = mac_res_get_dev(0);
    if (OAL_PTR_NULL == pst_mac_device) {
        OAM_ERROR_LOG0(0, OAM_SF_ACS, "{hmac_chr_get_chip_info::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    if (0 == pst_mac_device->uc_vap_num) {
        return OAL_FAIL;
    }

    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++) {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap) {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (IS_LEGACY_STA(pst_mac_vap)) {
            /* 原子接口 */
            hmac_chr_get_disasoc_reason(&pst_hmac_chr_info->st_disasoc_reason);
            hmac_chr_get_del_ba_info(pst_mac_vap, &pst_hmac_chr_info->st_del_ba_info);
            hmac_chr_get_connect_code(&pst_hmac_chr_info->us_connect_code);
            hmac_chr_get_vap_info(pst_mac_vap, &pst_hmac_chr_info->st_vap_info);
            hmac_chr_get_wifi_ext_info_from_device(pst_mac_vap, &pst_hmac_chr_info->st_wifi_ext_info, chr_event_id);  //通过device获取额外信息,需要上层适配
            CHR_EXCEPTION_P (chr_event_id, (oal_uint8 *)(pst_hmac_chr_info), OAL_SIZEOF(hmac_chr_info));
        }
    }

    /* 清除全局变量的历史值 */
    hmac_chr_info_clean();

    return OAL_SUCC;
}

oal_uint32 hmac_get_chr_info_event_hander(oal_uint32 chr_event_id)
{
    oal_uint32 ul_ret = 0;

    switch (chr_event_id) {
        case CHR_WIFI_DISCONNECT_QUERY_EVENTID:
        case CHR_WIFI_CONNECT_FAIL_QUERY_EVENTID:
        case CHR_WIFI_WEB_FAIL_QUERY_EVENTID:
        case CHR_WIFI_WEB_SLOW_QUERY_EVENTID:
            ul_ret = hmac_chr_get_chip_info(chr_event_id);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_chr_get_chip_info::hmac_chr_get_web_fail_slow_info fail.}");
                return ul_ret;
            }
            break;
        default:
            break;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_get_wifi_info_ext(hmac_get_wifi_info_ext_stru *pst_get_wifi_info_ext)
{
    oal_uint8                       uc_vap_index;
    mac_vap_stru                    *pst_mac_vap;
    mac_device_stru                 *pst_mac_device;
    hmac_vap_stru                   *pst_hmac_vap;
    oal_cali_param_stru             *pst_cali_data;
    hmac_device_stru               *pst_hmac_device;
    hal_btcoex_btble_status_stru   *pst_btble_status;

    if (pst_get_wifi_info_ext == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_get_wifi_info_ext::pst_get_wifi_info_ext null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(0);
    if (pst_mac_device == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (pst_mac_vap == OAL_PTR_NULL)
        {
            continue;
        }

        if (!IS_LEGACY_STA(pst_mac_vap))
        {
            continue;
        }

        pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
        if (pst_hmac_vap == OAL_PTR_NULL)
        {
            continue;
        }

#ifdef _PRE_WLAN_DFT_STAT
        /*获取device侧上报的抗干扰参数*/
        pst_get_wifi_info_ext->uc_device_distance = pst_hmac_vap->uc_device_distance;
        pst_get_wifi_info_ext->uc_intf_state_cca = pst_hmac_vap->uc_intf_state_cca;
        pst_get_wifi_info_ext->uc_intf_state_co  = pst_hmac_vap->uc_intf_state_co;
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
        /* 获取共存btble等状态 */
        pst_btble_status = &pst_hmac_device->st_hmac_device_btcoex.st_btble_status;
        oal_memcopy(&pst_get_wifi_info_ext->st_bt_status, &pst_btble_status->un_bt_status.st_bt_status, OAL_SIZEOF(chr_bt_status_stru));
        oal_memcopy(&pst_get_wifi_info_ext->st_ble_status, &pst_btble_status->un_ble_status.st_ble_status, OAL_SIZEOF(chr_ble_status_stru));
#endif
        /* 获取芯片类型 SS FF TT       */
        pst_cali_data = (oal_cali_param_stru *)get_cali_data_buf_addr();
        if (pst_cali_data != OAL_PTR_NULL)
        {
            pst_get_wifi_info_ext->uc_chip_type = pst_cali_data->st_bfgn_cali_data.uc_chip_type_code;
        }
    }
    return OAL_SUCC;
}

oal_void hmac_chr_connect_fail_query_and_report(hmac_vap_stru *pst_hmac_vap, mac_status_code_enum_uint16 connet_code)
{
    mac_chr_connect_fail_report_stru st_chr_connect_fail_report = { 0 };

    if (IS_LEGACY_STA(&pst_hmac_vap->st_vap_base_info)) {
        /* 主动查询 */
        hmac_chr_set_connect_code(connet_code);
        /* 主动上报 */
#ifdef CONFIG_HW_GET_EXT_SIG
        st_chr_connect_fail_report.ul_noise = pst_hmac_vap->station_info.noise;
        st_chr_connect_fail_report.ul_chload = pst_hmac_vap->station_info.chload;
#endif
        st_chr_connect_fail_report.c_signal = pst_hmac_vap->station_info.signal;
        st_chr_connect_fail_report.uc_distance = pst_hmac_vap->st_station_info_extend.uc_distance;
        st_chr_connect_fail_report.uc_cca_intr = pst_hmac_vap->st_station_info_extend.uc_cca_intr;

        st_chr_connect_fail_report.us_err_code = connet_code;
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_chr_get_chip_info::hmac_chr_get_web_fail_slow_info fail.}");
        CHR_EXCEPTION_P (CHR_WIFI_CONNECT_FAIL_REPORT_EVENTID, (oal_uint8 *)(&st_chr_connect_fail_report), OAL_SIZEOF(mac_chr_connect_fail_report_stru));
    }

    return;
}

#endif

oal_uint32 oam_cfg_init(oal_void)
{
    oal_int32 l_ret = OAL_SUCC;

    l_ret = oam_cfg_get_all_item();

    return (oal_uint32)l_ret;
}

oal_uint32 hmac_dfx_init(void)
{
    oam_register_init_hook(OM_WIFI, oam_cfg_init);
#ifdef _PRE_WLAN_1102A_CHR
    chr_host_callback_register(hmac_get_chr_info_event_hander);
    chr_get_wifi_ext_info_callback_register(hmac_get_wifi_info_ext);
#endif
    hmac_custom_init(OAL_FALSE);
    return OAL_SUCC;
}

oal_uint32 hmac_dfx_exit(void)
{
#ifdef _PRE_WLAN_1102A_CHR
    chr_host_callback_unregister();
    chr_get_wifi_ext_info_callback_unregister();
#endif
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_DFR
oal_module_symbol(g_st_dfr_info);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
