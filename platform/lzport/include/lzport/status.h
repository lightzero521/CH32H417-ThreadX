#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    LZPORT_OK = 0,
    LZPORT_EINVAL = -1,
    LZPORT_EBUSY = -2,
    LZPORT_EIO = -3,
    LZPORT_ETIMEOUT = -4
} lzport_status;

#ifdef __cplusplus
}
#endif
