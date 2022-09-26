#ifndef GALAXY_CAM_H
#define GALAXY_CAM_H

#include "ext_funcs.h"
/*#if !define(__cplusplus)
   #define bool _Bool
#endif*/
#include "GxIAPI.h"
#include "DxImageProc.h"

#define SUCCESS                           0
#define ERR_MALLOC                     -128
#define ERR_ENUMERATE_DEVICE           -127
#define ERR_GET_INFO_LIST              -126
#define ERR_NO_SUCH_DEVICE             -125
#define ERR_GALAXY_OPEN_FAILED         -124
#define ERR_SET_CONTINUOUS_ACQUISITION -123
#define ERR_SET_TRIGGER_MODE   -       -122
#define ERR_GET_IMG_WIDTH              -121
#define ERR_GET_IMG_HEIGHT             -120
#define ERR_GET_PAYLOAD_SIZE           -119
#define ERR_GET_PIXEL_COLOR_FORMAT     -118
#define ERR_GALAXY_NULLPTR             -117
#define ERR_INVALID_TRIGGER_MODE       -116
#define ERR_GET_SN_FAILED              -115
#define ERR_UNREGISTER_FAILED          -114
#define ERR_CLOSE_FAILED               -113
#define ERR_UNSET_CALLBACK_FAILED      -112

typedef enum {
    trigger_none = 0,
    trigger_software = 1,
    trigger_line0 = 2,
    trigger_line1 = 3,
    trigger_line2 = 4,
    trigger_line3 = 5
} trigger_mode_t;

typedef GX_DEV_HANDLE galaxy_cam_handle_t;
typedef GX_FRAME_DATA galaxy_frame_t;
typedef GX_DEVICE_BASE_INFO galaxy_cam_info_t;
typedef galaxy_cam_info_t *galaxy_cam_info_list_t;
typedef GX_FRAME_CALLBACK_PARAM *galaxy_captured_frame_t;

typedef void(GX_STDC *galaxy_capture_callback_t)(galaxy_captured_frame_t);

typedef GX_OPEN_PARAM galaxy_open_param_t;
typedef DX_PIXEL_COLOR_FILTER pixel_color_format_t;
typedef GX_FEATURE_ID_CMD galaxy_feature_ID_t;
typedef GX_STATUS galaxy_status_t;
typedef struct {
    galaxy_capture_callback_t callback;
    galaxy_cam_handle_t device_handle;
    galaxy_frame_t device_frame_data;
    int64_t img_height;
    int64_t img_width;
    int64_t payload_size;
    pixel_color_format_t pixel_color_format;
    char SN[13];//Assume all SNs are 12-byte long
} galaxy_cam_t;

__EXPORT_C_START

int32_t load_galaxy_lib(void);

int32_t unload_galaxy_lib(void);

int32_t update_cam_list(void);

uint32_t get_cam_amount(void);

int32_t get_galaxy_cam_SN(uint32_t device_index, char *device_SN);

int32_t open_galaxy_cam_by_SN(galaxy_cam_t *device, char *device_SN);

int32_t open_galaxy_cam_by_index(galaxy_cam_t *device, uint32_t device_index);

int32_t galaxy_lib_get_last_error(galaxy_status_t *err_code, char *err_info, size_t *buffer_size);

int32_t set_galaxy_cam_trigger_mode(galaxy_cam_t *device, trigger_mode_t trigger_mode);

int32_t get_galaxy_cam_float_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, double *value);

int32_t get_galaxy_cam_int_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t *value);

int32_t get_galaxy_cam_enum_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t *value);

int32_t set_galaxy_cam_float_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, double value);

int32_t set_galaxy_cam_int_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t value);

int32_t set_galaxy_cam_enum_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t value);

int32_t galaxy_cam_start_acquisition(galaxy_cam_t *device);

int32_t galaxy_cam_stop_acquisition(galaxy_cam_t *device);

int32_t unset_galaxy_cam_callback(galaxy_cam_t *device);

int32_t set_galaxy_cam_callback(galaxy_cam_t *device, galaxy_capture_callback_t device_callback, _Bool force_set);

int32_t galaxy_cam_send_software_trigger_command(galaxy_cam_t *device);

int32_t close_galaxy_cam(galaxy_cam_t *device);

__EXPORT_C_END

#endif
