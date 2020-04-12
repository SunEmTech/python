
#ifdef CONFIG_P2P
#include <ieee80211_defs.h> /* IEEE80211_FC0_SUBTYPE_BEACON, ... */
#include <wlan_dev.h>
#include "p2p_internal.h"
#include <pool_mgr_api.h>
#include <offload_mgr.h>
#include <wmi_unified.h>
#include <resmgr_api.h>
#include <tbd.h>
#include <wal_beacon.h>
#include <wlan_beacon_tx_offload_api.h>
#include <ar_wal.h>
#include <wlan_mgmt_local_txrx.h>
#include <wlan_framegen.h>
#include <wlan_peer.h>
#include <wal_phy_dev.h>
#include <sm.h> /* sm_dispatch */

#define GET_BE32(a) ((((A_UINT32) (a)[0]) << 24) | (((A_UINT32) (a)[1]) << 16) | \
        (((A_UINT32) (a)[2]) << 8) | ((A_UINT32) (a)[3]))

A_BOOL p2p_gather_p2pnoa_ie(A_UINT8 * start_frame, A_UINT8 * end_frame, A_UINT16 len, P2P_SUB_ELEM_NOA * noa);
A_UINT8 p2p_get_noa_ie(A_UINT8 * p2p_attr_start, A_UINT8 * p2p_attr_end,P2P_SUB_ELEM_NOA * noa);
void p2p_update_schedules(P2P_DEV_CTXT_S * p2p_dev_ctx);
void p2p_clear_schedules(P2P_DEV_CTXT_S * p2p_dev_ctx);
void p2p_update_start_time(P2P_DEV_CTXT_S * p2p_dev_ctx, A_UINT8 start_index);
A_UINT64 p2p_getTSF_Offset(A_UINT64 DevTSF, A_UINT64 FreeTSF, A_UINT8 * PositiveDelta);
void p2p_go_schedule_tsf_timeout(A_HANDLE alarm,void *arg);
void p2p_setup_schedule_timer(P2P_DEV_CTXT_S * p2p_dev_ctx);
void p2p_sort_schedule(P2P_GO_SCHEDULE * go_sched);
void p2p_process_schedule(
   P2P_GO_SCHEDULE     *go_schedule,
   A_UINT32            curr_tsf,
   A_BOOL              *one_shot_noa
   );
A_UINT16
p2p_calc_schedules(
    P2P_GO_SCHEDULE            *go_schedule,
    A_UINT32                   curr_tsf,
    A_UINT32                   *next_timeout,
    A_BOOL                     *GO_awake,
    A_INT16                    *highest_active_index);

void
p2p_find_all_next_events(
    P2P_GO_SCHEDULE             *go_schedule,
    A_UINT32                    event_time,
    A_INT16                     *highest_pri_active,
    A_INT16                     *earliest_next_event);

A_BOOL
p2p_get_GO_wake_state(
    P2P_GO_SCHEDULE     *go_schedule,
    A_INT16            priority);

A_UINT16 p2p_find_earliest_next_event(
    P2P_GO_SCHEDULE             *go_schedule,
    A_INT16                    start_idx);

A_BOOL
p2p_find_next_event(
    P2P_GO_SCHEDULE         *go_schedule,
    A_UINT32                curr_tsf,           /* current TSF Time */
    P2P_GO_SCHEDULE_REQ     *schedule,          /* current NOA or Presence Request */
    A_BOOL                  *is_active,         /* Return to indicate if NOA/Presence is active right now */
    A_UINT32                *next_event_time);   /* Return the next event time; can be the time to stop or start NOA */

void p2p_no_more_events(P2P_GO_SCHEDULE  *go_schedule);

 void
p2p_notify_GO_state_change(
    P2P_GO_SCHEDULE            *go_schedule,
    A_BOOL                     GO_awake,
    A_BOOL                     one_shot_noa,    /* Is this a one-shot NOA? */
    A_BOOL                     forced_callback,  /* Do a callback even if state is not changed */
    A_BOOL                     bOppPS          /* Whether to honour the state change */
    );

 void p2p_wal_pdev_event_handler(wal_pdev_t *wal_pdev,
        wal_phy_dev_event *event, void *event_arg);

 void p2p_go_noa_notif_handler (P2P_NOA_DESCRIPTOR *noa_data,
         A_UINT8 num_noa_desc, void *arg);

A_UINT32 p2p_adjust_noa_start_time(wlan_vdev_t *vdev, A_UINT32 wal_noa_start_time);

A_UINT8 p2p_find_noa_next_state_change_time(P2P_GO_SCHEDULE * go_schedule, A_UINT32 * next_change_time);

A_UINT32 p2p_find_noa_min_presence_dur(P2P_GO_SCHEDULE * go_schedule);

void p2p_update_rc_time_limit(wlan_vdev_t * vdevp, A_UINT32 min_presence_dur);

TXRX_STATUS p2p_local_send_noa_action(wlan_vdev_t *vdev);

void p2p_send_action_frame_complete_handler(void *ctxt, wal_peer_t *peer_handle,
                                     struct ath_buf *abf,WAL_FRAME_COMPLETION_STATUS completion_status,
                                     wal_tx_comp_desc_t *p_tx_comp_desc);

void p2p_go_noa_end_chreq_cb(resmgr_ocs_ch_req_t *req, resmgr_ocs_chreq_event_t ev, void *arg);

resmgr_ocs_ch_req_t * p2p_setup_hp_req_noa_end(wlan_vdev_t *vdev, A_UINT32  noa_end_time);

#ifdef WLAN_ENABLE_DIAG_EVENT_LOG_V2 
void p2p_noa_change_wdiag_log(wlan_vdev_t * vdev, A_UINT8 noaEvent, A_UINT32 next_timeout);
#endif
static void p2p_vdev_evt_handler(wlan_vdev_t *vdev, wlan_vdev_notif *notif, void *arg);

p2p_ctxt_t * gp2pCtxt;

EXR_INIT_TEXT void p2p_init(void)
{
    p2p_wmi_register();

    gp2pCtxt = (p2p_ctxt_t *)A_ALLOCRAM(sizeof(p2p_ctxt_t));

    gp2pCtxt->p2p_pool = pool_init(sizeof(P2P_DEV_CTXT_S),P2P_MAX_P2P_VDEVS_SUPPORTED,
    		POOL_ALLOC_AFTER_INIT | POOL_CLEAR_MEM_ON_FREE|POOL_SRAM_ALLOC);
    gp2pCtxt->p2p_ie_oui_type = P2P_IE_WFA_OUI_TYPE;
    gp2pCtxt->p2p_noa_attribute = P2P_WFA_NOA_ATTRIBUTE;
    gp2pCtxt->p2p_ps_sm_shared = p2p_ps_sm_create();
    A_PANIC(gp2pCtxt->p2p_ps_sm_shared);
}

void p2p_set_noa_rehandle(wlan_vdev_t *vdev, A_BOOL enable)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S *)vdev->p2p_dev_ctxt;
    p2p_dev_ctx->rehandle = enable;
}

void p2p_handle_NOA(A_BOOL NoaPresent, P2P_SUB_ELEM_NOA * noa, P2P_DEV_CTXT_S * p2p_dev_ctx)
{
    if(!p2p_dev_ctx) {
        return;
    }

    if(NoaPresent == TRUE)
    {
        int index;

        for(index = 0; index < noa->num_descriptors; index++)
        {
            if ((noa->noa_descriptors[index].type_count != 1)
                && (!WAL_TIME_IS_SMALLER(noa->noa_descriptors[index].duration, 
                        noa->noa_descriptors[index].interval)))
            {
                wdiag_msg(WLAN_MODULE_P2P, DBGLOG_ERR,
                        "P2P_HANDLE_NOA: Reject Invalid NOA, duration=%d,"
                        "interval=%d start_time=0x%x type_count=%d",
                        noa->noa_descriptors[index].duration,
                        noa->noa_descriptors[index].interval,
                        noa->noa_descriptors[index].start_time,
                        noa->noa_descriptors[index].type_count);

                return;
            }
        }

        //if this is either the first time we are seeing the NOA IE or the index
        //has changed or the other modules ask rehandle NOA explicitly we would
        //update the internal data structures.
        if((p2p_dev_ctx->noa_initialized == FALSE) ||
            (p2p_dev_ctx->rehandle == TRUE) ||
            (noa->index != p2p_dev_ctx->noa.index))
        {
            wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, "P2P_HANDLE_NOA: NoaPresent = %d",
                      NoaPresent);

            if (p2p_dev_ctx->rehandle == TRUE) {
                p2p_dev_ctx->rehandle = FALSE;
            }

            A_MEMCPY(&p2p_dev_ctx->noa,noa,sizeof(P2P_SUB_ELEM_NOA));
            p2p_dev_ctx->noa_initialized = TRUE;
            
            /* Update P2P NOA schedules and setup timer only when MAC is awake */
            if(wal_phy_dev_get_power_state(WAL_PDEV_FROM_WLAN_VDEV(p2p_dev_ctx->vdevp)) == 
                    WAL_POWER_STATE_AWAKE) 
            {
                p2p_update_schedules(p2p_dev_ctx);
            }
        }

    }
    else
    {
        if(p2p_dev_ctx->noa_initialized)
        {
            wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, "P2P_HANDLE_NOA: NoaPresent = %d",
                      NoaPresent);
            
            p2p_dev_ctx->noa_initialized = FALSE;
            A_MEMSET(&p2p_dev_ctx->noa,0,sizeof(P2P_SUB_ELEM_NOA));
            p2p_update_schedules(p2p_dev_ctx);
        }
    }
}

void p2p_get_noa_info(wlan_vdev_t * vdev, wmi_p2p_noa_info * noa_info)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    p2p_powersave_t * ps_handle = &p2p_dev_ctx->ps_handle;
    A_UINT8 num_descriptors = 0;
    int i;

    if(!p2p_dev_ctx) {
        return;
    }
    A_MEMZERO(noa_info,sizeof(wmi_p2p_noa_info));

    /* If the NOA has changed since the last time it was advertized,
     * provide the updated NOA.
     */
    if(ps_handle->index !=
            p2p_dev_ctx->adv_noa_index) {
        WMI_UNIFIED_NOA_ATTR_MODIFIED_SET(noa_info);
        WMI_UNIFIED_NOA_ATTR_INDEX_SET(noa_info, ps_handle->index);
        /* Update the advertized NOA index */
        p2p_dev_ctx->adv_noa_index = ps_handle->index;

        if( P2P_IS_OPPPS_ENABLE(ps_handle) ) {
            WMI_UNIFIED_NOA_ATTR_OPP_PS_SET(noa_info);
            WMI_UNIFIED_NOA_ATTR_CTWIN_SET(noa_info, ps_handle->ctwindow);
        }

        for(i =1; i < P2P_MAX_NOA_DESCRIPTORS + 1; i++) {
            if(!P2P_SCH_NOA_IS_INVALID(&(ps_handle->sch_descs[i]))) {
                noa_info->noa_descriptors[num_descriptors].type_count = ps_handle->sch_descs[i].count;
                noa_info->noa_descriptors[num_descriptors].duration = ps_handle->sch_descs[i].duration;
                noa_info->noa_descriptors[num_descriptors].interval = ps_handle->sch_descs[i].interval;
                noa_info->noa_descriptors[num_descriptors].start_time = p2p_adjust_noa_start_time(vdev,ps_handle->sch_descs[i].start);

                 wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
                           "P2P_GO_GET_NOA_INFO  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
						   num_descriptors,
                           p2p_dev_ctx->tbtt_offset,
                           ps_handle->sch_descs[i].start,
                           noa_info->noa_descriptors[num_descriptors].type_count,
                           noa_info->noa_descriptors[num_descriptors].duration,
                           noa_info->noa_descriptors[num_descriptors].interval,
                           noa_info->noa_descriptors[num_descriptors].start_time);


                 num_descriptors++;
            }
        }

        WMI_UNIFIED_NOA_ATTR_NUM_DESC_SET(noa_info, num_descriptors);
    }
}

void p2p_go_noa_end_chreq_cb(resmgr_ocs_ch_req_t *req, resmgr_ocs_chreq_event_t ev, void *arg)
{
    wlan_vdev_t *vdev = (wlan_vdev_t *)arg;
    P2P_DEV_CTXT_S *p2p_dev_ctx = NULL;

    if (vdev){
        p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    }

    if (p2p_dev_ctx == NULL) {
        return;
    }

    switch(ev) {
    case RESMGR_OCS_CHREQ_GRANT_EVENT:
        break;
    case RESMGR_OCS_CHREQ_COMPLETED_EVENT:
    case RESMGR_OCS_CHREQ_STOPPED_EVENT:
    case RESMGR_OCS_CHREQ_PREEMPTED_EVENT:
        if (p2p_dev_ctx->go_sec_hp_ch_req)
        {
            resmgr_ocs_ch_req_delete(vdev->wlan_pdev->resmgr_ctxt,
                                 p2p_dev_ctx->go_sec_hp_ch_req);

            p2p_dev_ctx->go_sec_hp_ch_req = NULL;
        }

        break;
	default:
	    break;
    }
}

resmgr_ocs_ch_req_t * p2p_setup_hp_req_noa_end(wlan_vdev_t *vdev, A_UINT32  noa_end_time)
{
    resmgr_channel_t chan;
    ch_req_attr_t ch_req_attr;
    resmgr_ocs_ch_req_t *go_sec_hp_ch_req = NULL;
    resmgr_ocs_ch_req_t *hp_ch_req = vdev->hp_ch_req;
    A_UINT32 curr_wal_time, start_tsf, end_tsf;

    if (!hp_ch_req)
        return NULL;

    /* if noa end time is between GO's HP request start tsf and end tsf
     * and end time has some gap (>20ms) to the end tsf then submit a new request
     * set the start tsf 1ms later and end tsf to the same as GO primary HP req.
     */
    curr_wal_time = noa_end_time;

    resmgr_ocs_get_chreq_param(hp_ch_req, &ch_req_attr, &chan);

    start_tsf = ch_req_attr.start_tsf;

    /*calculate the real start tsf most close to now, start tsf shall be an ealier value, update it*/
    start_tsf += ((curr_wal_time - start_tsf)/ch_req_attr.interval) * ch_req_attr.interval;

    end_tsf = start_tsf + ch_req_attr.duration;

    if (WAL_TIME_IS_SMALLER(start_tsf, curr_wal_time) &&
          WAL_TIME_IS_GREATER(end_tsf, curr_wal_time) &&
          WAL_TIME_DIFF(end_tsf, curr_wal_time) > 20*1024)
    {
        A_MEMZERO(&ch_req_attr, sizeof(ch_req_attr));

        ch_req_attr.interval = 0;
        ch_req_attr.strict_start_time = 1;
        ch_req_attr.start_tsf = curr_wal_time + 1024;
        ch_req_attr.priority = RESMGR_OCS_CH_REQ_PRIO_HIGH;
        ch_req_attr.duration = end_tsf - ch_req_attr.start_tsf;

        go_sec_hp_ch_req = resmgr_ocs_ch_req_create(
                            vdev->wlan_pdev->resmgr_ctxt,
                            &chan,
                            &ch_req_attr,
                            p2p_go_noa_end_chreq_cb,
                            vdev, vdev->e_mac_id);
        resmgr_ocs_ch_req_start(vdev->wlan_pdev->resmgr_ctxt,
                                             go_sec_hp_ch_req);
        return go_sec_hp_ch_req;
    }

    return NULL;
}

A_BOOL p2p_go_determine_noa_start_time (wlan_vdev_t * vdev, A_UINT32 duration,
    A_UINT32 * start_time, A_BOOL use_round_up, A_UINT32 round_up_offset)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    A_UINT32 cur_one_shot_end_time = 0, cur_wal_time;
    P2P_NOA_DESCRIPTOR noa_data;
    A_UINT8 noa_index;
    A_UINT32 start_offset1, start_offset2;

    if(!p2p_dev_ctx) {
        return FALSE;
    }

    cur_wal_time = wal_get_curr_time32();

    cur_one_shot_end_time = p2p_find_cur_one_shot_noa_end_time(vdev);

    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, 
              "P2P_GO_ADD_ONE_SHOT_NOA  0x%x 0x%x 0x%x",
              cur_one_shot_end_time, use_round_up, round_up_offset);


    wlan_vdev_determine_noa_start_time (vdev, cur_one_shot_end_time, &noa_data.start_time);

    *start_time = noa_data.start_time;

    if(use_round_up) {
        /* Round up GO channel dwell time to n*bi_us only between twice scan of one passive channel
                * ChN(dwell_time_passive)->Go channel(n*bi_us)-> ChN(dwell_time_passive)
                */
        start_offset1 = noa_data.start_time - cur_wal_time;

        start_offset2 = A_ROUND_UP(start_offset1, TU_TO_USEC(100));
        start_offset2 += round_up_offset;
        if((start_offset2 - TU_TO_USEC(100)) > start_offset1) {
            start_offset1 = start_offset2 - TU_TO_USEC(100);
        } else {
            start_offset1 = start_offset2;
        }
        noa_data.start_time = *start_time = start_offset1 + cur_wal_time;
    }

    noa_data.type_count = 1 ;/* Single shot */
    noa_data.duration = duration;
    noa_data.interval = 0;

    noa_index = p2p_add_noa_desc(&p2p_dev_ctx->ps_handle, &noa_data, P2P_GO_SCH_TYPE_TARGET);

    /* Invoke the PS scheduler after NOA desc is added */
    if(noa_index != P2P_GO_INVALID_INDEX) {
        //p2p_go_schedule(&p2p_dev_ctx->ps_handle);
        wal_untimeout_us(&(p2p_dev_ctx->ps_handle.trig_schedtimer));
        wal_timeout_us(&(p2p_dev_ctx->ps_handle.trig_schedtimer), 0, 0);
    }

    /*currently only support one backup GO HP request*/
    if (p2p_dev_ctx->go_sec_hp_ch_req == NULL)
    {
        p2p_dev_ctx->go_sec_hp_ch_req
          = p2p_setup_hp_req_noa_end(vdev, (noa_data.start_time + noa_data.duration));
    }

    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, "P2P_GO_ADD_ONE_SHOT_NOA  0x%x 0x%x 0x%x 0x%x 0x%x",
              noa_index,
              noa_data.type_count, noa_data.duration, noa_data.interval,
              noa_data.start_time);


    return TRUE;
}

OFFLOAD_STATUS
p2p_beacon_action_rx_frame_handler(void *context, wal_peer_t *wal_peer, struct rxbfChain *rxbuf)
{
    struct ieee80211_frame *wh;
    A_UINT8 type, subtype;
    wlan_vdev_t * vdev;
    wlan_peer_t * wlan_peer;
    P2P_DEV_CTXT_S * p2p_dev_ctx;
    OFFLOAD_STATUS ret = OFFLOAD_STATUS_NONE;
    if(wal_peer) {
        wlan_peer = wal_peer_get_upeer(wal_peer);
        vdev = (wlan_vdev_t*)wlan_peer->vdev;
        p2p_dev_ctx = (P2P_DEV_CTXT_S *)vdev->p2p_dev_ctxt;

        if((p2p_dev_ctx) &&
            (p2p_dev_ctx->noaOffloaded) &&
            (vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT))
        {
            wh = (struct ieee80211_frame *)
                 (WLAN_BUF_START(rxbuf->head->bf_b.wlanBuf.rx));
            type = wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK;
            subtype = wh->i_fc[0] & IEEE80211_FC0_SUBTYPE_MASK;

            if(type == IEEE80211_FC0_TYPE_MGT)
            {
                switch(subtype)
                {
                    case IEEE80211_FC0_SUBTYPE_BEACON:
                    case IEEE80211_FC0_SUBTYPE_PROBE_RESP:
                    case IEEE80211_FCO_SUBTYPE_ACTION:
                    {
                        P2P_SUB_ELEM_NOA noa = {0};
                        A_BOOL NOAPresent;
                        A_UINT8 * start_frame = (A_UINT8 *)(&wh[1]);

                        A_UINT16 Length = WLAN_BUF_LENGTH(rxbuf->head->bf_b.wlanBuf.rx) -
                                            (sizeof(struct ieee80211_frame) + sizeof(A_UINT32));

                        A_UINT8 * end_frame = start_frame + Length;
                        /*parser noa action frame*/
                        if(subtype == IEEE80211_FCO_SUBTYPE_ACTION){
                            if((start_frame[0] == P2P_ACTION_VENDOR_SPECIFIC) &&
                               (GET_BE32(&start_frame[1]) == gp2pCtxt->p2p_ie_oui_type)&&
                               (start_frame[5] == P2P_VENSPEC_ACTION_NOA)){
                                  start_frame += 7;//p2p ie offset
                             }else{
                                 break;
                             }
                        }else{
                            start_frame += 12;
                        }
                        NOAPresent = p2p_gather_p2pnoa_ie(start_frame,end_frame,Length,&noa);
                        p2p_handle_NOA(NOAPresent,&noa,p2p_dev_ctx);
                    }
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return ret;
}

void p2p_wal_pdev_event_handler(wal_pdev_t *wal_pdev,
        wal_phy_dev_event *event, void *event_arg)
{
    A_UINT32 *vdev_map;
    A_UINT32 *tbttoffset_map;

    A_UINT32 vdev_id=0;
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S *)event_arg;
    wlan_vdev_t *vdev;
    wal_power_state power_state = 0;

    if(! p2p_dev_ctx || ! p2p_dev_ctx->vdevp) {
        return;
    }

    vdev = p2p_dev_ctx->vdevp;
    vdev_id = p2p_dev_ctx->vdevp->id;
    
    if(vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO) {
        switch (event->event_type) {
            case WAL_PDEV_EVENT_TBTT_OFFSET_UPDATE:
            {        
                vdev_map = event->u.event_tbtt_offset_update.vdev_map;
                tbttoffset_map = event->u.event_tbtt_offset_update.tbtt_offset;
                if(WAL_IS_SET_VDEV_ID_MAP(vdev_map, vdev_id)) {
                    p2p_dev_ctx->tbtt_offset = tbttoffset_map[vdev_id];
                    p2p_resync_oppps(p2p_dev_ctx->vdevp);
                    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
                            "P2P_GO_TBTT_OFFSET  0x%x",
                            p2p_dev_ctx->tbtt_offset);

                }

                break;
            }
            case WAL_PDEV_EVENT_SWBA:
            {
                vdev_map = event->u.event_swba.vdev_map;
                if(WAL_IS_SET_VDEV_ID_MAP(vdev_map, vdev_id)) {
                    p2p_notify_tbtt(vdev);
                }
                break;
            }
            default:
                break;
        }   
    }
    else if(vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT) {
        switch (event->event_type) {
            case WAL_PDEV_EVENT_PRE_POWER_STATE_CHANGE:
            {
                power_state = event->u.event_power_state_change.new_power_state;
                
                /* MAC is going into power collapse. Clear P2P NOA/OPPPS 
                 * schedules to disarm the P2P timers.
                 */
                if (power_state == WAL_POWER_STATE_SLEEP || 
                        power_state == WAL_POWER_STATE_FULL_SLEEP)
                {
                    p2p_clear_schedules(p2p_dev_ctx);
                }
                
                break;
            }
            case WAL_PDEV_EVENT_POST_POWER_STATE_CHANGE:
            {
                power_state = event->u.event_power_state_change.new_power_state;
                
                /* MAC is awake. hence setup OPPPS and NOA schedule and setup timers */
                if (power_state == WAL_POWER_STATE_AWAKE) {
                    p2p_client_recal_noa(vdev);
                }
                
                break;
            }
            default:
                break;
        }
		wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
                            "p2p_wal_pdev_event_handler: P2P Client NOA schedules event %x, power_state %x", event->event_type, power_state);
    }
    
    return;
}

void p2p_go_notify_periodic_noa (wlan_vdev_t * vdev,
        P2P_NOA_DESCRIPTOR *noa_data, A_UINT8 num_noa_desc)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    p2p_powersave_t *ps_handle;
    int i;
    A_UINT8 noa_idx;

    if(!p2p_dev_ctx) {
        return;
    }
    
    ps_handle = &p2p_dev_ctx->ps_handle;

    if(!noa_data) {
        /* NOA data is NULL, so cleanup all existing FW generated peridioc NOAs */
        for(noa_idx =1; noa_idx <= P2P_MAX_NOA_DESCRIPTORS; noa_idx++) {
            if(p2p_dev_ctx->go_fw_periodic_noa_idx_map & (1 << noa_idx)) {
                p2p_del_noa_desc(ps_handle, noa_idx, FALSE);
            }
        }
        
        p2p_dev_ctx->go_fw_periodic_noa_idx_map = 0;

        /* NOA descriptors for periodic NOA are deleted. So, send an event 
         * indicating NOA changed to the PS SM.
         */
        sm_dispatch(ps_handle->ps_sm_shared,&ps_handle->ps_sm_handle,ps_handle,
                P2P_PS_EVT_NOA_CHANGED, 0, NULL);
        return;
    }
        
    /* We can have only one periodic NOA in P2P descriptor. Hence, cleanup all 
     * existing peridioc NOAs
     */
    for(noa_idx =1; noa_idx <= P2P_MAX_NOA_DESCRIPTORS; noa_idx++) {
        if((p2p_dev_ctx->go_fw_periodic_noa_idx_map & (1 << noa_idx)) || 
                (P2P_SCH_NOA_IS_HOST(&(ps_handle->sch_descs[noa_idx])))) 
        {
            p2p_del_noa_desc(ps_handle, noa_idx, FALSE);
        }
    }
    
    p2p_dev_ctx->go_fw_periodic_noa_idx_map = 0;
    p2p_dev_ctx->go_host_periodic_noa_idx_map = 0;
    
    /* If P2P GPO advertises both OPPPS and periodic NOA in the beacon, 
     * a crash will be seen P2P Client based on QCA solution. This due to a 
     * limitation on P2P Client side while parsing NOA attribute that includes 
     * both periodic NOA and OPPPS. Hence, the P2P GO will disable host configured 
     * OPPPS and NOA when FW adds a periodic NOA.
     */
    P2P_SET_OPPPS_DISABLE(ps_handle);
    p2p_go_scheduler_set_oppps(ps_handle, 0, 0,
            P2P_GO_ENDLESS_PERIODIC_NOA_CNT);
    
    ps_handle->is_p2p_host_noa_conf = FALSE;
    
    p2p_go_disable_host_ps(ps_handle);
    
    for(i = 0; i < num_noa_desc;i++) {
        /* Call Function to store the NOA in the P2P PS Handle */
        noa_idx =  p2p_add_noa_desc(ps_handle, &noa_data[i], P2P_GO_SCH_TYPE_TARGET);

        wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, "p2p_go_notify_periodic_noa: "
                "noa_idx=%u, start_time=0x%x, duration=%u, interval=%u, type_count=%u",
                noa_idx, noa_data[i].start_time, noa_data[i].duration, 
                noa_data[i].interval, noa_data[i].type_count);

        if(noa_idx != P2P_GO_INVALID_INDEX) {
            p2p_dev_ctx->go_fw_periodic_noa_idx_map |= (1 << noa_idx);
        }
    }

    /* Invoke PS scheduler after NoA desc added */
    if(p2p_dev_ctx->go_fw_periodic_noa_idx_map) {
        //p2p_go_schedule(&p2p_dev_ctx->ps_handle);
        wal_untimeout_us(&ps_handle->trig_schedtimer);
        wal_timeout_us(&ps_handle->trig_schedtimer, 0, 0);
    }

    return;
}

void _wmi_p2p_cmd_hdlr(void *ctx, A_UINT32 cmd_id,
        A_UINT8 *buffer, A_INT32 length)
{
    wlan_pdev_t *pdev = g_pdev_p;
    wlan_vdev_t *vdev = NULL;

    switch( cmd_id )
    {
        case WMI_P2P_SET_VENDOR_IE_DATA_CMDID:
        {
            WMI_P2P_SET_VENDOR_IE_DATA_CMDID_param_tlvs *param_tlv = (WMI_P2P_SET_VENDOR_IE_DATA_CMDID_param_tlvs *)buffer;
            wmi_p2p_set_vendor_ie_data_cmd_fixed_param *cmd = param_tlv->fixed_param;
            if(cmd != NULL) {
                gp2pCtxt->p2p_ie_oui_type = cmd->p2p_ie_oui_type;
                gp2pCtxt->p2p_noa_attribute = cmd->p2p_noa_attribute;
            }
            break;
        }
		case WMI_P2P_SET_OPPPS_PARAM_CMDID:
		{
			WMI_P2P_SET_OPPPS_PARAM_CMDID_param_tlvs *param_tlv = 
			        (WMI_P2P_SET_OPPPS_PARAM_CMDID_param_tlvs *)buffer;
			wmi_p2p_set_oppps_cmd_fixed_param *cmd = param_tlv->fixed_param;
			
			if(cmd != NULL) {
			    vdev = wlan_vdev_find_vdev(pdev, cmd->vdev_id);
			    
			    if(vdev && !DEV_VDEV_IS_FREE(vdev) && DEV_VDEV_IS_UP(vdev) && 
			            vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO) 
			    {
			        p2p_ps_set_oppps(pdev, cmd, length);
			    }
			}
			break;
		}
		case WMI_FWTEST_P2P_SET_NOA_PARAM_CMDID:
		{
		    WMI_FWTEST_P2P_SET_NOA_PARAM_CMDID_param_tlvs *param_tlv = 
		            (WMI_FWTEST_P2P_SET_NOA_PARAM_CMDID_param_tlvs *)buffer;
		    wmi_p2p_set_noa_cmd_fixed_param *cmd = param_tlv->fixed_param;

		    if(cmd != NULL && param_tlv->noa_descriptor != NULL) {
		        vdev = wlan_vdev_find_vdev(pdev, cmd->vdev_id);

		        if(vdev && !DEV_VDEV_IS_FREE(vdev) && DEV_VDEV_IS_UP(vdev) && 
		                vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO) 
		        {
		            p2p_ps_set_noa(pdev, param_tlv->fixed_param, 
		                    param_tlv->noa_descriptor);
		        }
		    }
		    break;
		}
        default:
            break;
    }
}

static WMI_DISPATCH_ENTRY p2p_wmi_dispatch_entries[] = {
    {
        _wmi_p2p_cmd_hdlr,
        WMI_P2P_SET_VENDOR_IE_DATA_CMDID,
        0,
    },
    {
        _wmi_p2p_cmd_hdlr,
        WMI_P2P_SET_OPPPS_PARAM_CMDID,
        0,
    },
    {
        _wmi_p2p_cmd_hdlr,
        WMI_FWTEST_P2P_SET_NOA_PARAM_CMDID,
        0,
    },
};

WMI_DECLARE_DISPATCH_TABLE(p2p_wmi_commands,
        p2p_wmi_dispatch_entries);

EXR_INIT_TEXT void p2p_wmi_register()
{
    /* register for wmi commands */
   WMI_RegisterDispatchTable(&p2p_wmi_commands);
}

static void p2p_vdev_evt_handler(wlan_vdev_t *vdev, wlan_vdev_notif *notif, void *arg)
{
    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
            "p2p_vdev_evt_handler  vdev_id = %d, evt = %d", vdev->id, notif->type);

    switch (notif->type) {
    case WLAN_VDEV_NOTIF_PRE_MIGRATION: {
        if (WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO == vdev->ic_subopmode || 
                WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT == vdev->ic_subopmode) 
        {
            wal_phy_dev_unregister_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                    p2p_wal_pdev_event_handler,
                    (void *)vdev->p2p_dev_ctxt);
        }
        break;
    }
    case WLAN_VDEV_NOTIF_POST_MIGRATION:
    {
        if (WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO == vdev->ic_subopmode) {
            /* Register for WAL tbttoffset and SWBA events for P2P GO*/
            if (wal_phy_dev_register_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                    p2p_wal_pdev_event_handler,
                    (void *)vdev->p2p_dev_ctxt,
                    WAL_PDEV_EVENT_TBTT_OFFSET_UPDATE|WAL_PDEV_EVENT_SWBA) != WAL_EOK) 
            {
                    A_ASSERT(FALSE);
            }
        }
        else if  (WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT == vdev->ic_subopmode) {
            /* Register for WAL POWER State Change events. P2P Client NOA timers 
             * will be disarmed when MAC goes into power collapse. NOA schedules 
             * will be recalculated upon wakeup.
             */
            if (wal_phy_dev_register_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                    p2p_wal_pdev_event_handler,
                    (void *)vdev->p2p_dev_ctxt,
                    (WAL_PDEV_EVENT_PRE_POWER_STATE_CHANGE | 
                            WAL_PDEV_EVENT_POST_POWER_STATE_CHANGE)) != WAL_EOK)
            {
                A_ASSERT(FALSE);
            }
        }
        break;
    }
    default:
        break;
    }
}

void * p2p_dev_init(wlan_vdev_t * vdev)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx;

    vdev->p2p_dev_ctxt = pool_alloc(gp2pCtxt->p2p_pool);
    A_PANIC(vdev->p2p_dev_ctxt);
    p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    
    A_MEMZERO(p2p_dev_ctx,sizeof(P2P_DEV_CTXT_S));
    
    p2p_dev_ctx->vdevp = vdev;
    p2p_dev_ctx->go_schedules.dev_ctx = vdev;
    p2p_dev_ctx->tbtt_offset = 0;

    p2p_dev_ctx->go_sec_hp_ch_req = NULL;

    p2p_dev_ctx->ps_handle.p2p_dev_ctx = p2p_dev_ctx;
    /* Store the p2p ps sm allocator shared between all ps sm instances
     * inside the ps_handle.
     */
    p2p_dev_ctx->ps_handle.ps_sm_shared = gp2pCtxt->p2p_ps_sm_shared;
    p2p_ps_sm_init(&p2p_dev_ctx->ps_handle);

    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_VERBOSE,
              "P2P_DEV_REGISTER  vdev_id = %d", vdev->id);


    /* Depending on the fw-subopmode we would register the callback
     * to offload manager for processing of beacon/probe response/
     * management action frame for processing of NOA.
     */
    if(vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT)
    {
        offldmgr_register_nondata_offload(NON_PROTO_OFFLOAD,
                                  OFFLOAD_NOA_OFFLOAD,
                                  p2p_beacon_action_rx_frame_handler,
                                  NULL,
                                  OFFLOAD_FRAME_TYPE_MGMT_SUBTYPE_PROBE_RSP |
                                  OFFLOAD_FRAME_TYPE_MGMT_SUBTYPE_BEACON |
                                  OFFLOAD_FRAME_TYPE_MGMT_SUBTYPE_ACTION
                                 );
        p2p_dev_ctx->noaOffloaded = TRUE;
        p2p_dev_ctx->go_schedules.go_present = TRUE;
        wal_init_timer(&p2p_dev_ctx->go_schedules.h_tsftimer,
                p2p_go_schedule_tsf_timeout,vdev);
        
        /* Register for WAL POWER State Change events. P2P Client NOA timers 
         * will be disarmed when MAC goes into power collapse. NOA schedules 
         * will be recalculated upon wakeup.
         */
        if (wal_phy_dev_register_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                p2p_wal_pdev_event_handler,
                (void *)p2p_dev_ctx,
                (WAL_PDEV_EVENT_PRE_POWER_STATE_CHANGE | 
                        WAL_PDEV_EVENT_POST_POWER_STATE_CHANGE)) != WAL_EOK)
        {
            A_PANIC(FALSE);
        }
    }
    else if (vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO) {
        /* Register for WAL tbttoffset and SWBA events of the P2P GO. */
        if (wal_phy_dev_register_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                p2p_wal_pdev_event_handler,
                (void *)p2p_dev_ctx,
                WAL_PDEV_EVENT_TBTT_OFFSET_UPDATE|WAL_PDEV_EVENT_SWBA) != WAL_EOK) 
        {
                A_PANIC(FALSE);
        }

#ifdef BEACON_TX_OFFLOAD_ENABLED
        if (wlan_beacon_tx_offload_register_txdone_notification(vdev,
            p2p_notify_beacon_send_comp, (void *)vdev)){
			    A_PANIC(FALSE);
		}
#endif
        
        wlan_vdev_register_notif_handler(vdev, p2p_vdev_evt_handler, NULL);

        p2p_dev_ctx->go_fw_periodic_noa_idx_map = 0;
        p2p_dev_ctx->go_host_periodic_noa_idx_map = 0;
	    p2p_dev_ctx->noaActTxDone = TRUE;
    }
    return NULL;
}

void p2p_dev_deinit(wlan_vdev_t * vdev)
{

    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;	

    if(p2p_dev_ctx == NULL) {
       return;
    }
    if (vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_GO) {
        wal_phy_dev_unregister_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                p2p_wal_pdev_event_handler,
                (void *)p2p_dev_ctx);
#ifdef BEACON_TX_OFFLOAD_ENABLED
        wlan_beacon_tx_offload_unregister_txdone_notification(vdev,
            p2p_notify_beacon_send_comp, (void *)vdev);
#endif
        wlan_vdev_unregister_notif_handler(vdev, p2p_vdev_evt_handler, NULL);
    } else if(vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT) {
        offldmgr_deregister_nondata_offload(OFFLOAD_NOA_OFFLOAD);
        wal_phy_dev_unregister_event_handler(WAL_PDEV_FROM_WLAN_VDEV(vdev),
                p2p_wal_pdev_event_handler,
                (void *)p2p_dev_ctx);
    }

    if (p2p_dev_ctx->go_sec_hp_ch_req)
    {
        resmgr_ocs_ch_req_stop(vdev->wlan_pdev->resmgr_ctxt, p2p_dev_ctx->go_sec_hp_ch_req);
        resmgr_ocs_ch_req_delete(vdev->wlan_pdev->resmgr_ctxt, p2p_dev_ctx->go_sec_hp_ch_req);
        p2p_dev_ctx->go_sec_hp_ch_req = NULL;
    }

    wal_untimeout_us(&p2p_dev_ctx->ps_handle.trig_schedtimer);
    wal_untimeout_us(&p2p_dev_ctx->ps_handle.h_tsftimer);
    wal_untimeout_us(&p2p_dev_ctx->ps_handle.go_ps_beacon_timer);
    if(vdev->ic_subopmode == WMI_UNIFIED_VDEV_SUBTYPE_P2P_CLIENT) {
		p2p_no_more_events(&p2p_dev_ctx->go_schedules);
		p2p_dev_ctx->noaOffloaded = FALSE;
	}
    
    pool_free(gp2pCtxt->p2p_pool,p2p_dev_ctx);
    vdev->p2p_dev_ctxt = NULL;
}

A_UINT8 wlan_p2p_add_noa(wlan_vdev_t * vdev , P2P_NOA_DESCRIPTOR *noa_data, A_UINT8 noa_type)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    p2p_powersave_t *ps_handle;
    A_UINT32 curr_wal_time;
    A_UINT8 noa_idx = 0;

    if(!p2p_dev_ctx || !noa_data)
        return P2P_GO_INVALID_INDEX;

    ps_handle = &p2p_dev_ctx->ps_handle;

    curr_wal_time = wal_get_curr_time32();

    /* The NOA start time should be ahead of current time. */
    A_ASSERT(WAL_TIME_IS_GREATER_EQ(noa_data->start_time, curr_wal_time));

    if(noa_data->type_count>1){
        /* We can have only one periodic NOA in P2P descriptor. Hence, cleanup all 
         * existing peridioc NOAs
         */
        for(noa_idx =1; noa_idx <= P2P_MAX_NOA_DESCRIPTORS; noa_idx++) {
            if((p2p_dev_ctx->go_fw_periodic_noa_idx_map & (1 << noa_idx))
                    || (P2P_SCH_NOA_IS_HOST(&(ps_handle->sch_descs[noa_idx])))) {
                p2p_del_noa_desc(&p2p_dev_ctx->ps_handle, noa_idx, FALSE);
            }
        }

        p2p_dev_ctx->go_fw_periodic_noa_idx_map = 0;
        p2p_dev_ctx->go_host_periodic_noa_idx_map = 0;
        
        /* If P2P GPO advertises both OPPPS and periodic NOA in the beacon, 
         * a crash will be seen P2P Client based on QCA solution. This due to a 
         * limitation on P2P Client side while parsing NOA attribute that includes 
         * both periodic NOA and OPPPS. Hence, the P2P GO will disable host configured 
         * OPPPS and NOA when FW adds a periodic NOA.
         */
        P2P_SET_OPPPS_DISABLE(ps_handle);
        p2p_go_scheduler_set_oppps(ps_handle, 0, 0,
                P2P_GO_ENDLESS_PERIODIC_NOA_CNT);
        
        ps_handle->is_p2p_host_noa_conf = FALSE;
        
        p2p_go_disable_host_ps(ps_handle);
    }

    /* Call Function to store the NOA in the P2P PS Handle */
    noa_idx = p2p_add_noa_desc(ps_handle, noa_data, noa_type);

    if((noa_data->type_count>1)&&(noa_idx != P2P_GO_INVALID_INDEX)){
        p2p_dev_ctx->go_fw_periodic_noa_idx_map |= (1 << noa_idx);
    }

    if(noa_idx != P2P_GO_INVALID_INDEX) {
        //p2p_go_schedule(ps_handle);
        wal_untimeout_us(&ps_handle->trig_schedtimer);
        wal_timeout_us(&ps_handle->trig_schedtimer, 0, 0);
    }

    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
            "wlan_p2p_add_noa: boa_idx=%u, start_time=0x%x, duration=%u, interval=%u"
            "type_count=%u", noa_idx, noa_data->start_time, noa_data->duration, 
            noa_data->interval, noa_data->type_count);

    return noa_idx;
}

A_STATUS wlan_p2p_del_noa(wlan_vdev_t * vdev , A_UINT8 index)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    p2p_powersave_t *ps_handle;

    if(!p2p_dev_ctx)
        return A_ERROR;

    ps_handle = &p2p_dev_ctx->ps_handle;

    return p2p_del_noa_desc(ps_handle, index, TRUE);
}

A_UINT16 wlan_p2p_get_noa_ie( wlan_vdev_t * vdev,A_UINT8 * noaptr)
{
    A_UINT8 *noa_start;
    A_UINT16 noa_ie_length=0;
    A_UINT8 index,oppPS,ctwindow,num_descriptors,i;
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    p2p_powersave_t * ps_handle = &p2p_dev_ctx->ps_handle;

    num_descriptors = 0;
    oppPS = 0;
    ctwindow = 0;
    index = 0;

    index = ps_handle->index;

    if(P2P_IS_OPPPS_ENABLE(ps_handle) ) {
        oppPS = TRUE;
        ctwindow = ps_handle->ctwindow;
    }

    num_descriptors = ps_handle->sch_desc_num;

    if((ctwindow == 0) && (oppPS == 0) &&(num_descriptors == 0))
    {
        return noa_ie_length;
    }else{
        A_UINT16 tmpUint16 = 0;
        A_UINT32 tmpUint32 = 0;
        
        noa_start = noaptr;
        *noaptr = P2P_WFA_NOA_ATTRIBUTE;
        noaptr++;

        tmpUint16 = A_HTOLE16(2 + num_descriptors*P2P_NOA_DESCRIPTOR_LEN);// 1 byte index + 1byte CTWindow and OppPS
        A_MEMCPY(noaptr, &tmpUint16, sizeof(A_UINT16));
        //*(A_UINT16 *)noaptr = A_HTOLE16(2 + num_descriptors*P2P_NOA_DESCRIPTOR_LEN);// 1 byte index + 1byte CTWindow and OppPS
        noaptr +=2;
        
        *noaptr = index;
        noaptr++;
        
        *noaptr = (oppPS ? ((ctwindow & P2P_NOA_IE_CTWIN_MASK) | P2P_NOA_IE_OPP_PS_SET) : 0);
        noaptr++;
        
        for (i =1; i < P2P_MAX_NOA_DESCRIPTORS + 1; i++) {
            if(!P2P_SCH_NOA_IS_INVALID(&(ps_handle->sch_descs[i]))){
                *noaptr = (A_UINT8)ps_handle->sch_descs[i].count;
                noaptr++;

                tmpUint32 = A_HTOLE32(ps_handle->sch_descs[i].duration);
                A_MEMCPY(noaptr, &tmpUint32, sizeof(A_UINT32));
                //*(A_UINT32 *)noaptr = A_HTOLE32(ps_handle->sch_descs[i].duration);
                noaptr += 4;

                tmpUint32 = A_HTOLE32(ps_handle->sch_descs[i].interval);
                A_MEMCPY(noaptr, &tmpUint32, sizeof(A_UINT32));
                //*(A_UINT32 *)noaptr = A_HTOLE32(ps_handle->sch_descs[i].interval);
                noaptr += 4;

                tmpUint32 = A_HTOLE32(p2p_adjust_noa_start_time(vdev,ps_handle->sch_descs[i].start));
                A_MEMCPY(noaptr, &tmpUint32, sizeof(A_UINT32));
                //*(A_UINT32 *)noaptr = A_HTOLE32(p2p_adjust_noa_start_time(vdev,ps_handle->sch_descs[i].start));
                noaptr += 4;

                wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
                          "P2P_GO_GET_NOA_IE  0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
                          p2p_dev_ctx->tbtt_offset,
                          ps_handle->sch_descs[i].start,
                          ps_handle->sch_descs[i].count,
                          ps_handle->sch_descs[i].duration,
                          ps_handle->sch_descs[i].interval,
                          p2p_adjust_noa_start_time(vdev, ps_handle->sch_descs[i].start));
            }
        }
        noa_ie_length = noaptr-noa_start;
    }

    return noa_ie_length;
}

A_UINT32 p2p_adjust_noa_start_time(wlan_vdev_t *vdev, A_UINT32 wal_noa_start_time)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;
    A_UINT32 curr_go_time, curr_wal_time, delta;
     /** The noa start time is stored w.r.t WAL timer. The GO uses a
        * free running TSF timer. Hence the NOA start time that is
        * stored in the GO PS handle is adjusted to follow the GO's
        * timer source. In addition,the TBTT offset is subtracted from
        * the start time.
        */
    curr_go_time = wal_vdev_get_tsf32((DEV_GET_WAL_VDEV(p2p_dev_ctx->vdevp)));
    curr_wal_time = wal_get_curr_time32();

    /* Usigned int math should take care of wraparound */
    delta = wal_noa_start_time -curr_wal_time;

    return  curr_go_time + delta - p2p_dev_ctx->tbtt_offset;
}

void p2p_deliver_event(wlan_vdev_t *vdev, wlan_p2p_event *event_data)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx;
    A_UINT32 idx;
    void *arg;

    if (NULL == vdev->p2p_dev_ctxt){
		A_ASSERT(FALSE);
		return;
	}
    p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;

    for(idx = 0; idx < p2p_dev_ctx->num_ev_handlers;idx++){
        if(p2p_dev_ctx->ev_handler_info[idx].evhandler){
            arg = p2p_dev_ctx->ev_handler_info[idx].arg;
            p2p_dev_ctx->ev_handler_info[idx].evhandler(vdev,event_data,arg);
        }
    }

}
A_STATUS p2p_register_event_handler(wlan_vdev_t *vdev,
                                                      wlan_p2p_event_handler evhandler,
                                                      void *arg)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx;
    A_UINT32 idx;

    if (NULL == vdev->p2p_dev_ctxt){
		A_ASSERT(FALSE);
		return A_EINVAL;
	}
    p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;

    /* use the handle that already exisits */
    for (idx = 0; idx < p2p_dev_ctx->num_ev_handlers;idx++) {
        if (p2p_dev_ctx->ev_handler_info[idx].evhandler == evhandler &&
            p2p_dev_ctx->ev_handler_info[idx].arg == arg) {
            return A_OK;
        }
    }

    if (p2p_dev_ctx->num_ev_handlers >= WLAN_P2P_REGISTER_EVENT_HANDLE_NUM) {
        return A_NO_MEMORY;
    }

    p2p_dev_ctx->ev_handler_info[p2p_dev_ctx->num_ev_handlers].evhandler = evhandler;
    p2p_dev_ctx->ev_handler_info[p2p_dev_ctx->num_ev_handlers].arg = arg;
    p2p_dev_ctx->num_ev_handlers++;

    return A_OK;
}

A_STATUS p2p_unregister_event_handler(wlan_vdev_t *vdev,
                                                      wlan_p2p_event_handler evhandler,
                                                      void *arg)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx;
    A_UINT32 idx;

    if (NULL == vdev->p2p_dev_ctxt){
		A_ASSERT(FALSE);
		return A_EINVAL;
	}
    p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;

    for (idx = 0; idx < p2p_dev_ctx->num_ev_handlers; idx++) {
        if (p2p_dev_ctx->ev_handler_info[idx].evhandler == evhandler &&
            p2p_dev_ctx->ev_handler_info[idx].arg == arg) {
            p2p_dev_ctx->ev_handler_info[idx] = p2p_dev_ctx->ev_handler_info[p2p_dev_ctx->num_ev_handlers-1];
            p2p_dev_ctx->num_ev_handlers--;

            return A_OK;
        }
    }

    return A_ENOENT;
}

/* Iterate over existing one-shot NOA descriptors and return the farthest end time.
 * If there are no one-shot NOA descriptors, return 0.
 */
A_UINT32 p2p_find_cur_one_shot_noa_end_time(wlan_vdev_t *vdev) {
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S *)vdev->p2p_dev_ctxt;
    p2p_powersave_t * ps_handle = &p2p_dev_ctx->ps_handle;
    A_UINT32 cur_one_shot_end_time = 0;
    p2p_go_sch_desc_t *pNext = NULL;

    for (pNext = ps_handle->pri_descs[P2P_GO_ONE_SHOT_NOA_PRIORITY];
            pNext; pNext = pNext->next) {
        A_UINT32 end_time = pNext->start + pNext->duration;

        A_ASSERT(pNext->count ==  1);

        cur_one_shot_end_time = ((end_time > cur_one_shot_end_time) ?
                end_time : cur_one_shot_end_time);

        wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
                  "P2P_CALC_SCHEDULES_CALL_ALL_NEXT_EVENT_FROM_WHILE_LOOP  0x%x 0x%x 0x%x",
                  pNext->start,
                  pNext->duration,
                  cur_one_shot_end_time);

    }

    return cur_one_shot_end_time;
}

A_UINT32 wlan_p2p_send_noa_action(wlan_vdev_t *vdev)
{
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;

    if(!p2p_dev_ctx) {
        A_ASSERT(FALSE);
		return A_ERROR;
    }

    //send noa action frame
    if((p2p_dev_ctx->noaActTxDone == TRUE)&&(!vdev->ifPaused))
    {
        if(p2p_local_send_noa_action(vdev) != TXRX_OK)
            A_ASSERT(FALSE);
        p2p_dev_ctx->noaActTxDone = FALSE;
    }else{
    /*to be discuss*/
    }

    return A_OK;
}

TXRX_STATUS p2p_local_send_noa_action(
        wlan_vdev_t *vdev)
{
    struct ath_buf *abf = NULL;
    wlan_peer_t * peer = vdev->bss_peer;
    wal_completion_args_t comp_args = {g_pdev_p, (WAL_LOCAL_COMPLETION_HANDLER*)p2p_send_action_frame_complete_handler};

    abf = wlan_frmgen_get_noa_action(vdev);
    if (NULL == abf) {
       return TXRX_ERROR;
    }

    return wlan_mgmt_local_send(vdev, peer, WAL_MGMT_TID, abf, PPDU_RATE_INVALID, &comp_args);

}

void p2p_send_action_frame_complete_handler(void *ctxt, wal_peer_t *peer_handle,
                                         struct ath_buf *abf,WAL_FRAME_COMPLETION_STATUS completion_status,
                                         wal_tx_comp_desc_t *p_tx_comp_desc)
{
    wlan_peer_t * wlan_peer = (wlan_peer_t *)peer_handle->wlan_peer;
    wlan_vdev_t * vdev = (wlan_vdev_t *)wlan_peer->vdev;
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;

    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO,
              "P2P_GO_GET_NOA_INFO  0x%x",
              completion_status);


    p2p_dev_ctx->noaActTxDone = TRUE;
}

A_UINT8  wlan_p2p_generate_noa_action(wlan_vdev_t *vdev,A_UINT8 *frmPtr)
{
    A_UINT8 *frm, *frm_start,*p2p_ie_len;
    A_UINT16 noa_length;
    A_UINT8 frame_len;

    frm_start = frm = frmPtr;

    /* add vender specific action frame header */
    *frm = P2P_ACTION_VENDOR_SPECIFIC;/*0x7f for IEEE802.11 vendor specific */
    frm ++;
    *frm = (A_UINT8)((((A_UINT32)P2P_IE_WFA_OUI_TYPE) >> 24) & 0xff);
    frm++;
    *frm = (A_UINT8)((((A_UINT32)P2P_IE_WFA_OUI_TYPE) >> 16) & 0xff);
    frm++;
    *frm = (A_UINT8)((((A_UINT32)P2P_IE_WFA_OUI_TYPE) >> 8) & 0xff);
    frm++;
    *frm = (A_UINT8)(((A_UINT32)P2P_IE_WFA_OUI_TYPE) & 0xff);
    frm++;
    *frm = P2P_VENSPEC_ACTION_NOA;/* OUI Subtype for Notice of Absence */
    frm ++;
    *frm = 0;/*dialog token 0 for notice of absence*/
    frm ++;

    /* add p2p ie header*/
    *frm = P2P_IE_WFA_ELEMENT_ID;
    frm ++;
    p2p_ie_len = frm;
    frm ++;
    *frm = (A_UINT8)((((A_UINT32)P2P_IE_WFA_OUI_TYPE) >> 24) & 0xff);
    frm++;
    *frm = (A_UINT8)((((A_UINT32)P2P_IE_WFA_OUI_TYPE) >> 16) & 0xff);
    frm++;
    *frm = (A_UINT8)((((A_UINT32)P2P_IE_WFA_OUI_TYPE) >> 8) & 0xff);
    frm++;
    *frm = (A_UINT8)(((A_UINT32)P2P_IE_WFA_OUI_TYPE) & 0xff);
    frm++;
    /* add noa subelement */
    noa_length = wlan_p2p_get_noa_ie(vdev,frm);
    frm += noa_length;
    /*update p2p ie length*/
    *p2p_ie_len = noa_length + 4;
    /*update frame length */
    frame_len = frm - frm_start;
    wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, "P2P_GO_GET_NOA_INFO  0x%x", frame_len);


    return frame_len;
}

void p2p_client_recal_noa(wlan_vdev_t *vdev) {
    P2P_DEV_CTXT_S * p2p_dev_ctx = (P2P_DEV_CTXT_S*)vdev->p2p_dev_ctxt;

    if(!p2p_dev_ctx) {
        return ;
    }

    if(p2p_dev_ctx->noa_initialized == TRUE) {
        wdiag_msg(WLAN_MODULE_P2P, DBGLOG_INFO, "p2p_client_recal_noa: vdev_id = %d ", 
                vdev->id);

        p2p_update_schedules(p2p_dev_ctx);
    }

}
#endif
