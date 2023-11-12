#ifndef BLUE_H
#define BLUE_H

#include "btstack.h"

#define RES_OK 0
#define RES_ERR 1

#define SBC_STORAGE_SIZE 1030
    
typedef struct {
    uint16_t a2dp_cid;
    uint8_t  local_seid;
    uint8_t  remote_seid;
    uint8_t  stream_opened;
    uint16_t avrcp_cid;

    uint32_t time_audio_data_sent; // ms
    uint32_t acc_num_missed_samples;
    uint32_t samples_ready;
    btstack_timer_source_t audio_timer;
    uint8_t  streaming;
    int      max_media_payload_size;
    uint32_t rtp_timestamp;

    uint8_t  sbc_storage[SBC_STORAGE_SIZE];
    uint16_t sbc_storage_count;
    uint8_t  sbc_ready_to_send;

    uint8_t volume;
} a2dp_media_sending_context_t;

typedef struct {
    int reconfigure;
    
    int num_channels;
    int sampling_frequency;
    int block_length;
    int subbands;
    int min_bitpool_value;
    int max_bitpool_value;
    btstack_sbc_channel_mode_t      channel_mode;
    btstack_sbc_allocation_method_t allocation_method;
} media_codec_configuration_sbc_t;

typedef struct {
    uint8_t track_id[8];
    uint32_t song_length_ms;
    avrcp_playback_status_t status;
    uint32_t song_position_ms; // 0xFFFFFFFF if not supported
} avrcp_play_status_info_t;

uint8_t blue_initialize(void);
uint8_t blue_start_scanning(void);
uint8_t blue_stop_scanning(void);
uint8_t blue_connect(char *addr);
uint8_t blue_start_streaming(void);

static void a2dp_source_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void a2dp_timer_start(a2dp_media_sending_context_t * context);
static void a2dp_demo_send_media_packet(void);

#endif