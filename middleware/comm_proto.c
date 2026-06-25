/**
 * @file    comm_proto.c
 * @brief   通信协议实现
 */

#include "comm_proto.h"
#include <string.h>

uint8_t proto_calc_checksum(const uint8_t *data, uint8_t len)
{
    uint8_t sum = PROTO_CHECKSUM_SEED;
    for (uint8_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

esp_err_t proto_build_frame(proto_frame_t *frame, proto_command_t command,
                            const uint8_t *data, uint8_t len)
{
    if (frame == NULL) return ESP_ERR_INVALID_ARG;
    if (len > sizeof(frame->data)) return ESP_ERR_INVALID_SIZE;

    memset(frame, 0, sizeof(proto_frame_t));
    frame->header = PROTO_FRAME_HEADER;
    frame->command = command;
    frame->length = len;

    if (data != NULL && len > 0) {
        memcpy(frame->data, data, len);
    }

    /* 校验和计算范围: command + length + data */
    frame->checksum = proto_calc_checksum(&frame->command, len + 2);

    return ESP_OK;
}

esp_err_t proto_parse_frame(const uint8_t *raw_data, uint8_t raw_len,
                            proto_frame_t *frame)
{
    if (raw_data == NULL || frame == NULL) return ESP_ERR_INVALID_ARG;
    if (raw_len < 4) return ESP_ERR_INVALID_SIZE;  /* 最小帧: header+cmd+len+checksum */

    /* 校验帧头 */
    if (raw_data[0] != PROTO_FRAME_HEADER) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(frame, raw_data, raw_len > sizeof(proto_frame_t) ? sizeof(proto_frame_t) : raw_len);

    /* 校验 checksum */
    uint8_t expected = proto_calc_checksum(&frame->command, frame->length + 2);
    if (frame->checksum != expected) {
        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}