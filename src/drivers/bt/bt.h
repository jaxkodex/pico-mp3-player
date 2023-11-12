#ifndef BT_H
#define BT_H

#include "btstack.h"

// logarithmic volume reduction, samples are divided by 2^x
// #define VOLUME_REDUCTION 3

//#define AVRCP_BROWSING_ENABLED

#define NUM_CHANNELS                2
#define BYTES_PER_AUDIO_SAMPLE      (2*NUM_CHANNELS)
#define AUDIO_TIMEOUT_MS            10 
#define TABLE_SIZE_441HZ            100

#define SBC_STORAGE_SIZE 1030

typedef enum {
    STREAM_SINE = 0,
    STREAM_MOD,
    STREAM_PTS_TEST
} stream_data_source_t;
    
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

int a2dp_source_and_avrcp_services_init(void);

/* @section Main Application Setup
 *
 * @text The Listing MainConfiguration shows how to setup AD2P Source and AVRCP services. 
 * Besides calling init() method for each service, you'll also need to register several packet handlers:
 * - hci_packet_handler - handles legacy pairing, here by using fixed '0000' pin code.
 * - a2dp_source_packet_handler - handles events on stream connection status (established, released), the media codec configuration, and, the commands on stream itself (open, pause, stopp).
 * - avrcp_packet_handler - receives connect/disconnect event.
 * - avrcp_controller_packet_handler - receives answers for sent AVRCP commands.
 * - avrcp_target_packet_handler - receives AVRCP commands, and registered notifications.
 * - stdin_process - used to trigger AVRCP commands to the A2DP Source device, such are get now playing info, start, stop, volume control. Requires HAVE_BTSTACK_STDIN.
 *
 * @text To announce A2DP Source and AVRCP services, you need to create corresponding
 * SDP records and register them with the SDP service. 
 */

/* LISTING_START(MainConfiguration): Setup Audio Source and AVRCP Target services */
static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void a2dp_source_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t * event, uint16_t event_size);
static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
#ifdef HAVE_BTSTACK_STDIN
static void stdin_process(char cmd);
#endif

static void a2dp_demo_hexcmod_configure_sample_rate(int sample_rate);

static void a2dp_demo_timer_start(a2dp_media_sending_context_t * context);
static void a2dp_demo_timer_stop(a2dp_media_sending_context_t * context);
static void a2dp_demo_send_media_packet(void);
static void a2dp_demo_audio_timeout_handler(btstack_timer_source_t * timer);

static int a2dp_demo_fill_sbc_audio_buffer(a2dp_media_sending_context_t * context);
static void produce_audio(int16_t * pcm_buffer, int num_samples);
static void produce_mod_audio(int16_t * pcm_buffer, int num_samples_to_write);
static void produce_sine_audio(int16_t * pcm_buffer, int num_samples_to_write);

#endif