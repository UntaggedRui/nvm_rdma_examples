#include <infiniband/verbs.h>
#include "Sherman/Debug.h"
void checkDMSupported(struct ibv_context *ctx)
{
    struct ibv_exp_device_attr attrs;
    int kMaxDeviceMemorySize = 0;

    attrs.comp_mask = IBV_EXP_DEVICE_ATTR_UMR;
    attrs.comp_mask |= IBV_EXP_DEVICE_ATTR_MAX_DM_SIZE;

    if (ibv_exp_query_device(ctx, &attrs))
    {
        Debug::notifyInfo("Couldn't query device attributes\n");
    }

    if (!(attrs.comp_mask & IBV_EXP_DEVICE_ATTR_MAX_DM_SIZE))
    {
        fprintf(stderr, "Can not support Device Memory!\n");
        exit(-1);
    }
    else if (!(attrs.max_dm_size))
    {
    }
    else
    {
        kMaxDeviceMemorySize = attrs.max_dm_size;
        Debug::notifyInfo("NIC Device Memory is %dKB\n", kMaxDeviceMemorySize / 1024);
    }
}

bool getnicmem()
{
    ibv_context *ctx = NULL;
    ibv_device *dev = NULL;
    // get device names in the system
    int devicesNum;
    uint8_t devIndex;

    struct ibv_device **deviceList = ibv_get_device_list(&devicesNum);
    if (!deviceList)
    {
        printf("failed to get IB devices list");
        goto CreateResourcesExit;
    }
    // if there isn't any IB device in host
    if (!devicesNum)
    {
        printf("found %d device(s)", devicesNum);
        goto CreateResourcesExit;
    }

    for (int i = 0; i < devicesNum; ++i)
    {
        // printf("Device %d: %s\n", i, ibv_get_device_name(deviceList[i]));
        if (ibv_get_device_name(deviceList[i])[5] == '0')
        {
            devIndex = i;
            break;
        }
    }

    dev = deviceList[devIndex];

    // get device handle
    ctx = ibv_open_device(dev);
    if (!ctx)
    {
        printf("failed to open device");
        goto CreateResourcesExit;
    }
    /* We are now done with device list, free it */
    ibv_free_device_list(deviceList);
    deviceList = NULL;
    checkDMSupported(ctx);
    ibv_close_device(ctx);
    return true;

CreateResourcesExit:
    return false;
}
int main()
{
    getnicmem();
    Debug::notifyInfo("complete");
    return 0;
}