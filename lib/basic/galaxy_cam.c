#include "galaxy_cam.h"

static uint32_t cam_amount;
static galaxy_cam_info_list_t cam_info_list = NULL;

__EXPORT_C_START

int32_t load_galaxy_lib(void) {
    if (cam_info_list != NULL)
        free(cam_info_list);
    cam_info_list = NULL;
    return GXInitLib();
}

int32_t unload_galaxy_lib(void) {
    if (cam_info_list != NULL)
        free(cam_info_list);
    cam_info_list = NULL;
    return GXCloseLib();
}

int32_t update_cam_list(void) {
    if (cam_info_list != NULL)
        free(cam_info_list);
    cam_info_list = NULL;
    galaxy_status_t status;
    if ((status = GXUpdateDeviceList(&cam_amount, 1000)) != GX_STATUS_SUCCESS)
        return ERR_ENUMERATE_DEVICE;
    size_t buffer_size = cam_amount * sizeof(galaxy_cam_info_t);
    cam_info_list = (galaxy_cam_info_list_t) (malloc(buffer_size));
    if (cam_info_list == NULL)
        return ERR_MALLOC;
    if ((status = GXGetAllDeviceBaseInfo(cam_info_list, &buffer_size)) != GX_STATUS_SUCCESS)
        return ERR_GET_INFO_LIST;
    return SUCCESS;
}

uint32_t get_cam_amount(void) {
    return cam_amount;
}


//Please ensure you have allocated enough space for device_SN
int32_t get_galaxy_cam_SN(uint32_t device_index, char *device_SN) {
    if (UNLIKELY(device_index >= cam_amount))
        return ERR_NO_SUCH_DEVICE;
    else {
        strcpy(device_SN, cam_info_list[device_index].szSN);
        return SUCCESS;
    }
}

//Note that 'device' has nothing (including its member 'SN') so you have to specify one
int32_t open_galaxy_cam_by_SN(galaxy_cam_t *device, char *device_SN) {
    galaxy_open_param_t open_param;
    open_param.accessMode = GX_ACCESS_EXCLUSIVE;
    open_param.openMode = GX_OPEN_SN;
    open_param.pszContent = device_SN;//Avoid extra time and space using by strcpy
    if (GXOpenDevice(&open_param, &device->device_handle) != GX_STATUS_SUCCESS)
        return ERR_GALAXY_OPEN_FAILED;
    strcpy(device->SN, device_SN);
    if (GXSetEnum(device->device_handle, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS) != GX_STATUS_SUCCESS)
        return ERR_SET_CONTINUOUS_ACQUISITION;
    if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF) !=
        GX_STATUS_SUCCESS) //Initially set trigger mode to none
        return ERR_SET_TRIGGER_MODE;
    if (GXGetInt(device->device_handle, GX_INT_WIDTH, &device->img_width) != GX_STATUS_SUCCESS)
        return ERR_GET_IMG_WIDTH;
    if (GXGetInt(device->device_handle, GX_INT_HEIGHT, &device->img_height) != GX_STATUS_SUCCESS)
        return ERR_GET_IMG_HEIGHT;
    if (GXGetInt(device->device_handle, GX_INT_PAYLOAD_SIZE, &device->payload_size) != GX_STATUS_SUCCESS)
        return ERR_GET_PAYLOAD_SIZE;
    if (GXGetEnum(device->device_handle, GX_ENUM_PIXEL_COLOR_FILTER, (int64_t *) (&device->pixel_color_format)) !=
        GX_STATUS_SUCCESS)
        return ERR_GET_PIXEL_COLOR_FORMAT;
    device->device_frame_data.pImgBuf = malloc(device->img_height * device->img_width);
    if (device->device_frame_data.pImgBuf == NULL)
        return ERR_MALLOC;
    device->callback = NULL;
    //All errors occur in following functions would be ignored but you can check GXGetLastErr()
    //GXSetEnum(device->device_handle, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_GREEN);
    GXSetEnum(device->device_handle, GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_OFF);
    GXSetEnum(device->device_handle, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, GX_ACQUISITION_FRAME_RATE_MODE_ON);
    GXSetEnum(device->device_handle, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);
    return SUCCESS;
}

int32_t open_galaxy_cam_by_index(galaxy_cam_t *device, uint32_t device_index) {
    ++device_index;
    if (device_index > cam_amount)
        return ERR_NO_SUCH_DEVICE;
    galaxy_open_param_t open_param;
    open_param.accessMode = GX_ACCESS_EXCLUSIVE;
    open_param.openMode = GX_OPEN_INDEX;
    char buffer[13];
    sprintf(buffer, "%d", device_index);
    open_param.pszContent = buffer;//Avoid extra time and space using by strcpy
    if (GXOpenDevice(&open_param, &device->device_handle) != GX_STATUS_SUCCESS)
        return ERR_GALAXY_OPEN_FAILED;
    if (get_galaxy_cam_SN(device_index - 1, buffer) != SUCCESS)
        return ERR_GET_SN_FAILED;
    strcpy(device->SN, buffer);
    if (GXSetEnum(device->device_handle, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS) != GX_STATUS_SUCCESS)
        return ERR_SET_CONTINUOUS_ACQUISITION;
    if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF) !=
        GX_STATUS_SUCCESS) //Initially set trigger mode to none
        return ERR_SET_TRIGGER_MODE;
    if (GXGetInt(device->device_handle, GX_INT_WIDTH, &device->img_width) != GX_STATUS_SUCCESS)
        return ERR_GET_IMG_WIDTH;
    if (GXGetInt(device->device_handle, GX_INT_HEIGHT, &device->img_height) != GX_STATUS_SUCCESS)
        return ERR_GET_IMG_HEIGHT;
    if (GXGetInt(device->device_handle, GX_INT_PAYLOAD_SIZE, &device->payload_size) != GX_STATUS_SUCCESS)
        return ERR_GET_PAYLOAD_SIZE;
    if (GXGetEnum(device->device_handle, GX_ENUM_PIXEL_COLOR_FILTER, (int64_t *) (&device->pixel_color_format)) !=
        GX_STATUS_SUCCESS)
        return ERR_GET_PIXEL_COLOR_FORMAT;
    device->device_frame_data.pImgBuf = malloc(device->img_height * device->img_width);
    if (device->device_frame_data.pImgBuf == NULL)
        return ERR_MALLOC;
    device->callback = NULL;
    //All errors occur in following functions would be ignored but you can check GXGetLastErr()
    //GXSetEnum(device->device_handle, GX_ENUM_BALANCE_RATIO_SELECTOR, GX_BALANCE_RATIO_SELECTOR_GREEN);
    GXSetEnum(device->device_handle, GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_OFF);
    GXSetEnum(device->device_handle, GX_ENUM_ACQUISITION_FRAME_RATE_MODE, GX_ACQUISITION_FRAME_RATE_MODE_ON);
    GXSetEnum(device->device_handle, GX_ENUM_EXPOSURE_AUTO, GX_EXPOSURE_AUTO_OFF);
    return SUCCESS;
}

int32_t galaxy_cam_register_capture_callback(galaxy_cam_t *device, galaxy_capture_callback_t capture_callback) {
    return GXRegisterCaptureCallback(device->device_handle, NULL, capture_callback);
}

int32_t galaxy_lib_get_last_error(galaxy_status_t *err_code, char *err_info, size_t *buffer_size) {
    if (err_info == NULL)
        return ERR_GALAXY_NULLPTR;
    return GXGetLastError(err_code, err_info, buffer_size);
}

int32_t set_galaxy_cam_trigger_mode(galaxy_cam_t *device, trigger_mode_t trigger_mode) {
    switch (trigger_mode) {
        case trigger_none:
            return GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_OFF);
        case trigger_software:
            if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON))
                return ERR_SET_TRIGGER_MODE;
            return GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_SOFTWARE);
        case trigger_line0:
            if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON))
                return ERR_SET_TRIGGER_MODE;
            return GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE0);
        case trigger_line1:
            if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON))
                return ERR_SET_TRIGGER_MODE;
            return GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE1);
        case trigger_line2:
            if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON))
                return ERR_SET_TRIGGER_MODE;
            return GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE2);
        case trigger_line3:
            if (GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_MODE, GX_TRIGGER_MODE_ON))
                return ERR_SET_TRIGGER_MODE;
            return GXSetEnum(device->device_handle, GX_ENUM_TRIGGER_SOURCE, GX_TRIGGER_SOURCE_LINE3);
    }
    return ERR_INVALID_TRIGGER_MODE;
}

int32_t get_galaxy_cam_float_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, double *value) {
    return GXGetFloat(device->device_handle, feature_ID, value);
}

int32_t get_galaxy_cam_int_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t *value) {
    return GXGetInt(device->device_handle, feature_ID, value);
}

int32_t get_galaxy_cam_enum_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t *value) {
    return GXGetEnum(device->device_handle, feature_ID, value);
}

int32_t set_galaxy_cam_float_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, double value) {
    return GXSetFloat(device->device_handle, feature_ID, value);
}

int32_t set_galaxy_cam_int_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t value) {
    return GXSetInt(device->device_handle, feature_ID, value);
}

int32_t set_galaxy_cam_enum_feature(galaxy_cam_t *device, galaxy_feature_ID_t feature_ID, int64_t value) {
    return GXSetEnum(device->device_handle, feature_ID, value);
}

int32_t galaxy_cam_start_acquisition(galaxy_cam_t *device) {
    return GXSendCommand(device->device_handle, GX_COMMAND_ACQUISITION_START);
}

int32_t galaxy_cam_stop_acquisition(galaxy_cam_t *device) {
    return GXSendCommand(device->device_handle, GX_COMMAND_ACQUISITION_STOP);
}

int32_t unset_galaxy_cam_callback(galaxy_cam_t *device) {
    galaxy_status_t status = GXUnregisterCaptureCallback(device->device_handle);
    if (status == GX_STATUS_SUCCESS) {
        device->callback = NULL;
    }
    return status;
}

int32_t set_galaxy_cam_callback(galaxy_cam_t *device, galaxy_capture_callback_t device_callback, _Bool force_set) {
    if (device->callback != NULL) {
        if (unset_galaxy_cam_callback(device) != GX_STATUS_SUCCESS && !force_set)
            return ERR_UNSET_CALLBACK_FAILED;
    }
    galaxy_status_t status = GXRegisterCaptureCallback(device->device_handle, NULL, device_callback);
    if (status == GX_STATUS_SUCCESS) {
        device->callback = device_callback;
    }
    return status;
}

int32_t galaxy_cam_send_software_trigger_command(galaxy_cam_t *device) {
    return GXSendCommand(device->device_handle, GX_COMMAND_TRIGGER_SOFTWARE);
}

int32_t close_galaxy_cam(galaxy_cam_t *device) {
    if (device->callback == NULL)
        if (GXUnregisterCaptureCallback(device->device_handle) != GX_STATUS_SUCCESS)
            return ERR_UNREGISTER_FAILED;
    if (device->device_frame_data.pImgBuf != NULL)free(device->device_frame_data.pImgBuf);
    device->device_frame_data.pImgBuf = NULL;
    if (GXCloseDevice(device->device_handle) != GX_STATUS_SUCCESS)
        return ERR_CLOSE_FAILED;
    return SUCCESS;
}

__EXPORT_C_END

