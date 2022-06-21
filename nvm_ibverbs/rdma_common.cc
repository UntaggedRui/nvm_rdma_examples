#include "rdma_common.hh"
bool createContext(RdmaContext *context, uint8_t port, int gidIndex,
                   uint8_t devIndex)
{
    ibv_device *dev = NULL;
    ibv_context *ctx = NULL;
    ibv_pd *pd = NULL;
    ibv_port_attr portAttr;
    int k = 0;


    // get device names in the system
    int devicesNum = 0;
    struct ibv_device **deviceList = ibv_get_device_list(&devicesNum);
    if (!deviceList)
    {
        Debug::notifyError("failed to get IB devices list");
        goto CreateResourcesExit;
    }

    // if there is not any IB device in host
    if (!devicesNum)
    {
        Debug::notifyInfo("found %d device(s)", devicesNum);
        goto CreateResourcesExit;
    }

    for (; k < devicesNum; k++) {
        ctx = ibv_open_device(deviceList[k]);
        if (ctx) {
            int rc = 0;
            for (int j=0; j < 2; j++) {
                int ret;
                ret = ibv_query_port(ctx, j, &portAttr);
                if (ret == 0) {
                    if (portAttr.state == IBV_PORT_ACTIVE) {
                        // Debug::notifyInfo("ib device %s, phys_state=%d, state=%d with port %d\n", 
                        // ibv_get_device_name(deviceList[i]), port_attr.phys_state, port_attr.state, j);
                        rc = 1;
                        devIndex = k;
                        break;
                    }
                }

            }
            if (rc) break;

        }
    }


    if (devIndex >= devicesNum)
    {
        Debug::notifyError("ib device wasn't found!");
        goto CreateResourcesExit;
    }

    dev = deviceList[devIndex];
    Debug::notifyInfo("open device %s: ", ibv_get_device_name(dev));
    ctx = ibv_open_device(dev);
    if (!ctx)
    {
        Debug::notifyError("failed to open device");
        goto CreateResourcesExit;
    }

    /* We are now done with device list, free it */
    ibv_free_device_list(deviceList);
    deviceList = NULL;

   


    // allocate Protection Domain
    pd = ibv_alloc_pd(ctx);
    if (!pd)
    {
        Debug::notifyError("ibv_alloc_pd failed");
        goto CreateResourcesExit;
    }

    if (ibv_query_gid(ctx, port, gidIndex, &context->gid))
    {
        Debug::notifyError("could not get gid for port :%d, gidIndex: %d", port, gidIndex);
        goto CreateResourcesExit;
    }

    context->devIndex = devIndex;
    context->gidIndex = gidIndex;
    context->port = port;
    context->ctx = ctx;
    context->pd = pd;
    context->lid = portAttr.lid;
    return true;
/* Error encountered, cleanup */
CreateResourcesExit:
    Debug::notifyError("Error Encountered, Cleanup ... wandan");

    if (pd)
    {
        ibv_dealloc_pd(pd);
        pd = NULL;
    }
    if (ctx)
    {
        ibv_close_device(ctx);
        ctx = NULL;
    }
    if (deviceList)
    {
        ibv_free_device_list(deviceList);
        deviceList = NULL;
    }

    return false;
}

bool destoryContext(RdmaContext *context)
{
    bool rc = true;
    if (context->pd)
    {
        if (ibv_dealloc_pd(context->pd))
        {
            Debug::notifyError("Failed to deallocate PD");
            rc = false;
        }
    }

    if (context->ctx)
    {
        if (ibv_close_device(context->ctx))
        {
            Debug::notifyError("failed to close device context");
            rc = false;
        }
    }
    return rc;
}
bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *send_cq,
                     ibv_cq *recv_cq, RdmaContext *context,
                     uint32_t qpsMaxDepth, uint32_t maxInlineData)
{
    struct ibv_exp_qp_init_attr attr;
    memset(&attr, 0, sizeof(attr));

    attr.qp_type = mode;
    attr.sq_sig_all = 0;
    attr.recv_cq = recv_cq;
    attr.send_cq = send_cq;
    attr.pd = context->pd;

    if (mode == IBV_QPT_RC)
    {
        attr.comp_mask = IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS | IBV_EXP_QP_INIT_ATTR_PD | IBV_EXP_QP_INIT_ATTR_ATOMICS_ARG;
        attr.max_atomic_arg = 32;
    }
    else
    {
        attr.comp_mask = IBV_EXP_QP_INIT_ATTR_PD;
    }

    attr.cap.max_send_wr = qpsMaxDepth;
    attr.cap.max_recv_wr = qpsMaxDepth;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_sge = 1;
    attr.cap.max_inline_data = maxInlineData;

    *qp = ibv_exp_create_qp(context->ctx, &attr);
    if (!(*qp))
    {
        Debug::notifyError("Failed to create QP");
        return false;
    }

    // Debug::notifyInfo("Create Queue Pair with Num = %d", (*qp)->qp_num);

    return true;
}
bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *cq,
                     RdmaContext *context, uint32_t qpsMaxDepth,
                     uint32_t maxInlineData)
{
    return createQueuePair(qp, mode, cq, cq, context, qpsMaxDepth, maxInlineData);
}
ibv_mr *createMemoryRegion(uint64_t mm, uint64_t mmSize, RdmaContext *ctx)
{
    ibv_mr *mr = NULL;
    mr = ibv_reg_mr(ctx->pd, (void *)mm, mmSize, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC);

    if (!mr)
    {
        Debug::notifyError("Memory registration failed");
    }

    return mr;
}
bool modifyQPtoInit(struct ibv_qp *qp, RdmaContext *context)
{
    struct ibv_qp_attr attr;
    struct ibv_qp_init_attr init_attr;

    if(ibv_query_qp(qp, &attr, IBV_QP_STATE, &init_attr) !=0)
    {
        Debug::notifyError("Failed to query QP.");
        return false;
    }
    if(attr.qp_state != IBV_QPS_RESET)
    {
        Debug::notifyError("Error: QP state not RESET when calling modify_qp_to_INIT().");
        return false;
    }
    memset(&attr, 0, sizeof(attr));

    attr.qp_state = IBV_QPS_INIT;
    attr.port_num = context->port;
    attr.pkey_index = 0;

    switch (qp->qp_type)
    {
    case IBV_QPT_RC:
        attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC;
        break;
    case IBV_QPT_UC:
        attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE;
        break;
    case IBV_EXP_QPT_DC_INI:
        Debug::notifyError("implement me:");
        break;

    default:
        Debug::notifyError("error qp_type");
    }

    if (ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS))
    {
        Debug::notifyError("Failed to modify QP state to INIT");
        return false;
    }
    return true;
}

void fillAhAttr(ibv_ah_attr *attr, struct Rdma_connect_info *remoteinfo)
{
    memset(attr, 0, sizeof(ibv_ah_attr));
    attr->dlid = remoteinfo->lid;
    attr->sl = 0;
    attr->src_path_bits = 0;
    attr->port_num = PORT_NUM;

    // fill ah_attr with GRH
    attr->is_global = 1;
    attr->grh.dgid = remoteinfo->gid;
    attr->grh.flow_label = 0;
    attr->grh.hop_limit = 1;
    attr->grh.sgid_index = 3;
    attr->grh.traffic_class = 0;
}
bool modifyQPtoRTR(struct ibv_qp *qp, struct Rdma_connect_info *remoteinfo)
{
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_4096;
    attr.dest_qp_num = remoteinfo->qpn;
    attr.rq_psn = PSN;

    fillAhAttr(&attr.ah_attr, remoteinfo);

    int flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN;

    if (qp->qp_type == IBV_QPT_RC)
    {
        attr.max_dest_rd_atomic = 1;
        attr.min_rnr_timer = 12;
        flags |= IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;
    }
    if (ibv_modify_qp(qp, &attr, flags))
    {
        Debug::notifyError("failed to modify QP state to RTR");
        return false;
    }
    return true;
}

bool modifyQPtoRTS(struct ibv_qp *qp)
{
    struct ibv_qp_attr attr;
    struct ibv_qp_init_attr init_attr;
    int flags;
    memset(&attr, 0, sizeof(attr));

    if(ibv_query_qp(qp, &attr, IBV_QP_STATE, &init_attr) != 0)
    {
        Debug::notifyError("Failed to query QP.");
        return false;
    }

    if(attr.qp_state != IBV_QPS_RTR)
    {
        Debug::notifyError("Error: QP state not RTR when calling modify_qp_to_RTS().");
        return false;
    }
    memset(&attr, 0, sizeof(struct ibv_qp_attr));

    attr.qp_state = IBV_QPS_RTS;
    attr.sq_psn = PSN;
    flags = IBV_QP_STATE | IBV_QP_SQ_PSN;

    if (qp->qp_type = IBV_QPT_RC)
    {
        attr.timeout = 14;
        attr.retry_cnt = 7;
        attr.rnr_retry = 7;
        attr.max_rd_atomic = 1;
        flags |= IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC;
    }
    int res = ibv_modify_qp(qp, &attr, flags);
    if (res)
    {
        Debug::notifyError("failed to modify QP state to RTS, res is %d", res);
        return false;
    }
    return true;
}

void wire_gid_to_gid(const char *wgid, union ibv_gid *gid)
{
    // char tmp[9];
    // uint32_t v32;
    // int i;

    // for (tmp[8] = 0, i = 0; i < 4; ++i)
    // {
    //     memcpy(tmp, wgid + i * 8, 8);
    //     sscanf(tmp, "%x", &v32);
    //     *(uint32_t *)(&gid->raw[i * 4]) = ntohl(v32);
    // }
    char tmp[9];
    uint32_t v32;
    uint32_t *raw = (uint32_t *)gid->raw;
    int i;

    for (tmp[8] = 0, i = 0; i < 4; ++i) {
        memcpy(tmp, wgid + i * 8, 8);
        sscanf(tmp, "%x", &v32);
        raw[i] = ntohl(v32);
    }
}
void gid_to_wire_gid(const union ibv_gid *gid, char wgid[])
{
    // int i;

    // for (i = 0; i < 4; ++i)
    //     sprintf(&wgid[i * 8], "%08x", htonl(*(uint32_t *)(gid->raw + i * 4)));

    int i;
    uint32_t *raw = (uint32_t *)gid->raw;

    for (i = 0; i < 4; ++i) {
        sprintf(&wgid[i * 8], "%08x", htonl(raw[i]));
    }
}
void ibv_gid_to_char(union ibv_gid *ibvgid, uint8_t *gid)
{
    memcpy((char *)gid, (char *)ibvgid, 16 * sizeof(uint8_t));
}
static inline void fillSgeWr(ibv_sge &sg, ibv_send_wr &wr, uint64_t source, uint64_t size, uint32_t lkey)
{
    memset(&sg, 0, sizeof(sg));
    sg.addr = (uintptr_t)source;
    sg.length = size;
    sg.lkey = lkey;

    memset(&wr, 0, sizeof(wr));
    wr.wr_id = 0;
    wr.sg_list = &sg;
    wr.num_sge = 1;
}
bool rdmaWrite(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
               uint32_t lkey, uint32_t remoteRKey, int32_t imm, bool isSignaled,
               uint64_t wrID)
{
    struct ibv_sge sg;
    struct ibv_send_wr wr;
    struct ibv_send_wr *badwr;

    fillSgeWr(sg, wr, source, size, lkey);

    if (imm = -1)
    {
        wr.opcode = IBV_WR_RDMA_WRITE;
    }
    else
    {
        wr.imm_data = imm;
        wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    }
    if (isSignaled)
    {
        wr.send_flags = IBV_SEND_SIGNALED;
    }
    wr.wr.rdma.remote_addr = dest;
    wr.wr.rdma.rkey = remoteRKey;
    wr.wr_id = wrID;

    if (ibv_post_send(qp, &wr, &badwr) != 0)
    {
        Debug::notifyError("Send with RDMA_WRITE(WITH_IMM) failed.");
        sleep(10);
        return false;
    }

    return true;
}
int pollWithCQ(ibv_cq *cq, int pollNumber, struct ibv_wc *wc)
{
    uint16_t count = 0;
    do
    {
        int new_count = ibv_poll_cq(cq, 1, wc);
        count += new_count;
    } while (count < pollNumber);

    if (count < 0)
    {
        Debug::notifyError("Poll Completion failed\n");
        sleep(5);
        return -1;
    }

    if (wc->status != IBV_WC_SUCCESS)
    {
        Debug::notifyError("Failed status %s(%d) for wr_id %d", ibv_wc_status_str(wc->status), wc->status, (int)wc->wr_id);
        sleep(5);
        return -1;
    }
    return count;
}

bool rdmaRead(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size, uint32_t lkey, uint32_t remoteRKey, bool signal, uint64_t wrID)
{
    struct ibv_sge sg;
    struct ibv_send_wr wr;
    struct ibv_send_wr *wrBad;

    fillSgeWr(sg, wr, source, size, lkey);

    wr.opcode = IBV_WR_RDMA_READ;
    if(signal)
    {
        wr.send_flags = IBV_SEND_SIGNALED;
    }
    wr.wr.rdma.remote_addr = dest;
    wr.wr.rdma.rkey = remoteRKey;
    wr.wr_id = wrID;

    if(ibv_post_send(qp, &wr, &wrBad))
    {
        Debug::notifyError("Send with RDMA_READ failed");
        return false;
    }
    return true;

}