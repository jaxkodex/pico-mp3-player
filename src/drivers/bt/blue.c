#include <stdio.h>
#include <inttypes.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

// #include "btstack.h"
// #include "hxcmod.h"
#include "blue.h"
#include "blue_utils.h"
#include "../../player.h"

#define AUDIO_TIMEOUT_MS   5

avrcp_play_status_info_t play_info;
static a2dp_media_sending_context_t media_tracker;
static media_codec_configuration_sbc_t sbc_configuration;
static btstack_sbc_encoder_state_t sbc_encoder_state;
static btstack_packet_callback_registration_t hci_event_callback_registration;

static uint8_t device_id_sdp_service_buffer[100];
static uint8_t sdp_a2dp_source_service_buffer[150];
static uint8_t sdp_avrcp_target_service_buffer[200];
static uint8_t sdp_avrcp_controller_service_buffer[200];

static int new_sample_rate = 44100;
static int current_sample_rate = 44100;
static const int INQUIRY_DURATION_1280MS = 12;

static uint8_t media_sbc_codec_configuration[4];
static  uint8_t media_sbc_codec_capabilities[] = {
    (AVDTP_SBC_44100 << 4) | AVDTP_SBC_STEREO,
    0xFF,//(AVDTP_SBC_BLOCK_LENGTH_16 << 4) | (AVDTP_SBC_SUBBANDS_8 << 2) | AVDTP_SBC_ALLOCATION_METHOD_LOUDNESS,
    2, 53
};

typedef struct {
  bool scan_active;
} Blue;

static Blue bt;

uint8_t blue_start_scanning(void){
  printf("Start scanning...\n");
  uint8_t res = gap_inquiry_start(INQUIRY_DURATION_1280MS);
  if (res == RES_OK) bt.scan_active = true;
  return res;
}

uint8_t blue_stop_scanning(void) {
  printf("Stop scanning...\n");
  uint8_t res = gap_inquiry_stop();
  if (res == RES_OK) bt.scan_active = false;
  return res;
}

uint8_t blue_connect(char *addr) {
  bd_addr_t device_addr;
  uint8_t status = ERROR_CODE_SUCCESS;
  gap_inquiry_stop();
  sscanf_bd_addr(addr, device_addr);
  status = a2dp_source_establish_stream(device_addr, &media_tracker.a2dp_cid);
  printf("Create A2DP Source connection to addr %s, cid 0x%02x. Result: %02x\n", bd_addr_to_str(device_addr), media_tracker.a2dp_cid, status);
  return status;
}

uint8_t blue_start_streaming(void) {
  printf("Start streaming...\n");
  uint8_t status = ERROR_CODE_SUCCESS;
  if (!media_tracker.stream_opened) {
    printf("Stream not opened. Can not play, configure a speaker first.\n");
    return status;
  }
  status = a2dp_source_start_stream(media_tracker.a2dp_cid, media_tracker.local_seid);
  printf("Start streaming. Result: %02x\n", status);
  return status;
}

uint8_t blue_initialize(void) {
  printf("Initialize Bluetooth...\n");

  // Request role change on reconnecting headset to always use them in slave mode
  hci_set_master_slave_policy(0);
  // enabled EIR
  hci_set_inquiry_mode(INQUIRY_MODE_RSSI_AND_EIR);

  l2cap_init();

  // Initialize  A2DP Source
  a2dp_source_init();
  a2dp_source_register_packet_handler(&a2dp_source_packet_handler);

    // Create stream endpoint
  avdtp_stream_endpoint_t * local_stream_endpoint = a2dp_source_create_stream_endpoint(AVDTP_AUDIO, AVDTP_CODEC_SBC, media_sbc_codec_capabilities, sizeof(media_sbc_codec_capabilities), media_sbc_codec_configuration, sizeof(media_sbc_codec_configuration));
  if (!local_stream_endpoint){
      printf("A2DP Source: not enough memory to create local stream endpoint\n");
      return RES_ERR;
  }
  printf("A2DP Source: Stream Endpoint created.\n");

  // Store stream enpoint's SEP ID, as it is used by A2DP API to indentify the stream endpoint
  media_tracker.local_seid = avdtp_local_seid(local_stream_endpoint);
  avdtp_source_register_delay_reporting_category(media_tracker.local_seid);
  printf("A2DP Source: Stream Endpoint ID %d registered.\n", media_tracker.local_seid);

  // Initialize AVRCP Service
  avrcp_init();
  avrcp_register_packet_handler(&avrcp_packet_handler);
  printf("AVRCP: service registered.\n");
  // Initialize AVRCP Target
  avrcp_target_init();
  avrcp_target_register_packet_handler(&avrcp_target_packet_handler);
  printf("AVRCP Target: service registered.\n");

  // Initialize AVRCP Controller
  avrcp_controller_init();
  avrcp_controller_register_packet_handler(&avrcp_controller_packet_handler);
  printf("AVRCP Controller: service registered.\n");

  // Initialize SDP, 
  sdp_init();
  printf("SDP service started.\n");
  
  // Create A2DP Source service record and register it with SDP
  memset(sdp_a2dp_source_service_buffer, 0, sizeof(sdp_a2dp_source_service_buffer));
  a2dp_source_create_sdp_record(sdp_a2dp_source_service_buffer, 0x10001, AVDTP_SOURCE_FEATURE_MASK_PLAYER, NULL, NULL);
  sdp_register_service(sdp_a2dp_source_service_buffer);
  printf("A2DP Source service registered.\n");

  // Create AVRCP Target service record and register it with SDP. We receive Category 1 commands from the headphone, e.g. play/pause
  memset(sdp_avrcp_target_service_buffer, 0, sizeof(sdp_avrcp_target_service_buffer));
  uint16_t supported_features = AVRCP_FEATURE_MASK_CATEGORY_PLAYER_OR_RECORDER;

  avrcp_target_create_sdp_record(sdp_avrcp_target_service_buffer, 0x10002, supported_features, NULL, NULL);
  sdp_register_service(sdp_avrcp_target_service_buffer);
  printf("AVRCP Target service registered.\n");
#ifdef AVRCP_BROWSING_ENABLED
    supported_features |= AVRCP_FEATURE_MASK_BROWSING;
#endif
    avrcp_target_create_sdp_record(sdp_avrcp_target_service_buffer, 0x10002, supported_features, NULL, NULL);
    sdp_register_service(sdp_avrcp_target_service_buffer);
    printf("AVRCP Target service registered.\n");

    // Create AVRCP Controller service record and register it with SDP. We send Category 2 commands to the headphone, e.g. volume up/down
    memset(sdp_avrcp_controller_service_buffer, 0, sizeof(sdp_avrcp_controller_service_buffer));
    uint16_t controller_supported_features = AVRCP_FEATURE_MASK_CATEGORY_MONITOR_OR_AMPLIFIER;
    avrcp_controller_create_sdp_record(sdp_avrcp_controller_service_buffer, 0x10003, controller_supported_features, NULL, NULL);
    sdp_register_service(sdp_avrcp_controller_service_buffer);
    printf("AVRCP Controller service registered.\n");

    // Register Device ID (PnP) service SDP record
    memset(device_id_sdp_service_buffer, 0, sizeof(device_id_sdp_service_buffer));
    device_id_create_sdp_record(device_id_sdp_service_buffer, 0x10004, DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH, BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, 1, 1);
    sdp_register_service(device_id_sdp_service_buffer);
    printf("Device ID service registered.\n");

    // Set local name with a template Bluetooth address, that will be automatically
    // replaced with a actual address once it is available, i.e. when BTstack boots
    // up and starts talking to a Bluetooth module.
    gap_set_local_name("A2DP Source 00:00:00:00:00:00");
    gap_discoverable_control(1);
    gap_set_class_of_device(0x200408);
    printf("Set local name.\n");
    
    // Register for HCI events.
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    printf("Register HCI callback.\n");

    printf("Configure sample rate");
    uint8_t sample_rate_status = a2dp_source_reconfigure_stream_sampling_frequency(media_tracker.a2dp_cid, 48000);
    printf("Configure sample rate result: %02x\n", sample_rate_status);

    return RES_OK;
}

static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
  UNUSED(channel);
  UNUSED(size);
  if (packet_type != HCI_EVENT_PACKET) return;
  uint8_t status;
  UNUSED(status);

  bd_addr_t address;
  uint32_t cod;

  // Service Class: Rendering | Audio, Major Device Class: Audio
  const uint32_t bluetooth_speaker_cod = 0x200000 | 0x040000 | 0x000400;

  const uint32_t type = hci_event_packet_get_type(packet);

//   printf("HCI packet handler, type: %d\n", type);

  switch (type){
      case HCI_EVENT_PIN_CODE_REQUEST:
          printf("Pin code request - using '0000'\n");
          hci_event_pin_code_request_get_bd_addr(packet, address);
          gap_pin_code_response(address, "0000");
          break;
      case GAP_EVENT_INQUIRY_RESULT:
          gap_event_inquiry_result_get_bd_addr(packet, address);
          char address_str[18];
          memcpy(address_str, bd_addr_to_str(address), 18);
          // print info
          printf("Device found: %s ",  address_str);
          cod = gap_event_inquiry_result_get_class_of_device(packet);
          printf("with COD: %06" PRIx32, cod);
          if (gap_event_inquiry_result_get_rssi_available(packet)){
              printf(", rssi %d dBm", (int8_t) gap_event_inquiry_result_get_rssi(packet));
          }
          char name_buffer[240];
          if (gap_event_inquiry_result_get_name_available(packet)){
              int name_len = gap_event_inquiry_result_get_name_len(packet);
              memcpy(name_buffer, gap_event_inquiry_result_get_name(packet), name_len);
              name_buffer[name_len] = 0;
              printf(", name '%s'", name_buffer);
          } else {
            memcpy(name_buffer, "Unknown", 8);
          }
          printf("\n");
          if ((cod & bluetooth_speaker_cod) == bluetooth_speaker_cod){
            //   bd_addr_t device_addr;
            //   memcpy(device_addr, address, 6);
            //   printf("Bluetooth speaker detected, trying to connect to %s...\n", address_str);
          }
          add_blue_device(name_buffer, address_str);
          break;
      case GAP_EVENT_INQUIRY_COMPLETE:
          if (bt.scan_active){
            printf("Inquiry scan complete.\n");
            bt.scan_active = false;
            //   printf("No Bluetooth speakers found, scanning again...\n");
            //   blue_start_scanning();
          }
          break;
      default:
          break;
  }
}

static void a2dp_source_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    uint8_t status;
    uint8_t local_seid;
    bd_addr_t address;
    uint16_t cid;

    avdtp_channel_mode_t channel_mode;
    uint8_t allocation_method;

    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_A2DP_META) return;

    switch (hci_event_a2dp_meta_get_subevent_code(packet)){
        case A2DP_SUBEVENT_SIGNALING_CONNECTION_ESTABLISHED:
            a2dp_subevent_signaling_connection_established_get_bd_addr(packet, address);
            cid = a2dp_subevent_signaling_connection_established_get_a2dp_cid(packet);
            status = a2dp_subevent_signaling_connection_established_get_status(packet);

            if (status != ERROR_CODE_SUCCESS){
                printf("A2DP Source: Connection failed, status 0x%02x, cid 0x%02x, a2dp_cid 0x%02x \n", status, cid, media_tracker.a2dp_cid);
                media_tracker.a2dp_cid = 0;
                break;
            }
            media_tracker.a2dp_cid = cid;
            media_tracker.volume = 32;

            printf("A2DP Source: Connected to address %s, a2dp cid 0x%02x, local seid 0x%02x.\n", bd_addr_to_str(address), media_tracker.a2dp_cid, media_tracker.local_seid);
            break;

         case A2DP_SUBEVENT_SIGNALING_MEDIA_CODEC_SBC_CONFIGURATION:{
            cid  = avdtp_subevent_signaling_media_codec_sbc_configuration_get_avdtp_cid(packet);
            if (cid != media_tracker.a2dp_cid) return;

            media_tracker.remote_seid = a2dp_subevent_signaling_media_codec_sbc_configuration_get_remote_seid(packet);
            
            sbc_configuration.reconfigure = a2dp_subevent_signaling_media_codec_sbc_configuration_get_reconfigure(packet);
            sbc_configuration.num_channels = a2dp_subevent_signaling_media_codec_sbc_configuration_get_num_channels(packet);
            sbc_configuration.sampling_frequency = a2dp_subevent_signaling_media_codec_sbc_configuration_get_sampling_frequency(packet);
            sbc_configuration.block_length = a2dp_subevent_signaling_media_codec_sbc_configuration_get_block_length(packet);
            sbc_configuration.subbands = a2dp_subevent_signaling_media_codec_sbc_configuration_get_subbands(packet);
            sbc_configuration.min_bitpool_value = a2dp_subevent_signaling_media_codec_sbc_configuration_get_min_bitpool_value(packet);
            sbc_configuration.max_bitpool_value = a2dp_subevent_signaling_media_codec_sbc_configuration_get_max_bitpool_value(packet);
            
            channel_mode = (avdtp_channel_mode_t) a2dp_subevent_signaling_media_codec_sbc_configuration_get_channel_mode(packet);
            allocation_method = a2dp_subevent_signaling_media_codec_sbc_configuration_get_allocation_method(packet);
            
            printf("A2DP Source: Received SBC codec configuration, sampling frequency %u, a2dp_cid 0x%02x, local seid 0x%02x, remote seid 0x%02x.\n", 
                sbc_configuration.sampling_frequency, cid,
                   a2dp_subevent_signaling_media_codec_sbc_configuration_get_local_seid(packet),
                   a2dp_subevent_signaling_media_codec_sbc_configuration_get_remote_seid(packet));
            
            // Adapt Bluetooth spec definition to SBC Encoder expected input
            sbc_configuration.allocation_method = (btstack_sbc_allocation_method_t)(allocation_method - 1);
            switch (channel_mode){
                case AVDTP_CHANNEL_MODE_JOINT_STEREO:
                    printf("A2DP Source: Channel mode: Joint Stereo\n");
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_JOINT_STEREO;
                    break;
                case AVDTP_CHANNEL_MODE_STEREO:
                    printf("A2DP Source: Channel mode: Stereo\n");
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_STEREO;
                    break;
                case AVDTP_CHANNEL_MODE_DUAL_CHANNEL:
                    printf("A2DP Source: Channel mode: Dual Channel\n");
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_DUAL_CHANNEL;
                    break;
                case AVDTP_CHANNEL_MODE_MONO:
                    printf("A2DP Source: Channel mode: Mono\n");
                    sbc_configuration.channel_mode = SBC_CHANNEL_MODE_MONO;
                    break;
                default:
                    btstack_assert(false);
                    break;
            }

            btstack_sbc_encoder_init(&sbc_encoder_state, SBC_MODE_STANDARD, 
                sbc_configuration.block_length, sbc_configuration.subbands, 
                sbc_configuration.allocation_method, sbc_configuration.sampling_frequency, 
                sbc_configuration.max_bitpool_value,
                sbc_configuration.channel_mode);
            break;
        }  

        case A2DP_SUBEVENT_SIGNALING_DELAY_REPORTING_CAPABILITY:
            printf("A2DP Source: remote supports delay report, remote seid %d\n", 
                avdtp_subevent_signaling_delay_reporting_capability_get_remote_seid(packet));
            break;
        case A2DP_SUBEVENT_SIGNALING_CAPABILITIES_DONE:
            printf("A2DP Source: All capabilities reported, remote seid %d\n", 
                avdtp_subevent_signaling_capabilities_done_get_remote_seid(packet));
            break;

        case A2DP_SUBEVENT_SIGNALING_DELAY_REPORT:
            printf("A2DP Source: Received delay report of %d.%0d ms, local seid %d\n", 
                avdtp_subevent_signaling_delay_report_get_delay_100us(packet)/10, avdtp_subevent_signaling_delay_report_get_delay_100us(packet)%10,
                avdtp_subevent_signaling_delay_report_get_local_seid(packet));
            break;
       
        case A2DP_SUBEVENT_STREAM_ESTABLISHED:
            a2dp_subevent_stream_established_get_bd_addr(packet, address);
            status = a2dp_subevent_stream_established_get_status(packet);
            if (status != ERROR_CODE_SUCCESS){
                printf("A2DP Source: Stream failed, status 0x%02x.\n", status);
                break;
            }
            
            local_seid = a2dp_subevent_stream_established_get_local_seid(packet);
            cid = a2dp_subevent_stream_established_get_a2dp_cid(packet);
            
            printf("A2DP Source: Stream established a2dp_cid 0x%02x, local_seid 0x%02x, remote_seid 0x%02x\n", cid, local_seid, a2dp_subevent_stream_established_get_remote_seid(packet));
            
            // TODO: Replace with new sample rate for mp3
            // a2dp_demo_hexcmod_configure_sample_rate(current_sample_rate);
            media_tracker.stream_opened = 1;
            status = a2dp_source_start_stream(media_tracker.a2dp_cid, media_tracker.local_seid);
            break;

        case A2DP_SUBEVENT_STREAM_RECONFIGURED:
            status = a2dp_subevent_stream_reconfigured_get_status(packet);
            local_seid = a2dp_subevent_stream_reconfigured_get_local_seid(packet);
            cid = a2dp_subevent_stream_reconfigured_get_a2dp_cid(packet);

            if (status != ERROR_CODE_SUCCESS){
                printf("A2DP Source: Stream reconfiguration failed, status 0x%02x\n", status);
                break;
            }

            printf("A2DP Source: Stream reconfigured a2dp_cid 0x%02x, local_seid 0x%02x\n", cid, local_seid);
            // TODO: Replace with new sample rate for mp3
            // a2dp_demo_hexcmod_configure_sample_rate(new_sample_rate);
            status = a2dp_source_start_stream(media_tracker.a2dp_cid, media_tracker.local_seid);
            break;

        case A2DP_SUBEVENT_STREAM_STARTED:
            local_seid = a2dp_subevent_stream_started_get_local_seid(packet);
            cid = a2dp_subevent_stream_started_get_a2dp_cid(packet);

            play_info.status = AVRCP_PLAYBACK_STATUS_PLAYING;
            if (media_tracker.avrcp_cid){
                // TODO: set now playing info
                // avrcp_target_set_now_playing_info(media_tracker.avrcp_cid, &tracks[data_source], sizeof(tracks)/sizeof(avrcp_track_t));
                avrcp_target_set_playback_status(media_tracker.avrcp_cid, AVRCP_PLAYBACK_STATUS_PLAYING);
            }
            // TODO: start timer - start playing
            a2dp_timer_start(&media_tracker);
            printf("A2DP Source: Stream started, a2dp_cid 0x%02x, local_seid 0x%02x\n", cid, local_seid);
            break;

        case A2DP_SUBEVENT_STREAMING_CAN_SEND_MEDIA_PACKET_NOW:
            local_seid = a2dp_subevent_streaming_can_send_media_packet_now_get_local_seid(packet);
            cid = a2dp_subevent_signaling_media_codec_sbc_configuration_get_a2dp_cid(packet);
            printf("Sending data\n");
            a2dp_demo_send_media_packet();
            break;        

        case A2DP_SUBEVENT_STREAM_SUSPENDED:
            local_seid = a2dp_subevent_stream_suspended_get_local_seid(packet);
            cid = a2dp_subevent_stream_suspended_get_a2dp_cid(packet);
            
            play_info.status = AVRCP_PLAYBACK_STATUS_PAUSED;
            if (media_tracker.avrcp_cid){
                avrcp_target_set_playback_status(media_tracker.avrcp_cid, AVRCP_PLAYBACK_STATUS_PAUSED);
            }
            printf("A2DP Source: Stream paused, a2dp_cid 0x%02x, local_seid 0x%02x\n", cid, local_seid);
            
            // TODO: stop timer - stop playing
            // a2dp_demo_timer_stop(&media_tracker);
            break;

        case A2DP_SUBEVENT_STREAM_RELEASED:
            play_info.status = AVRCP_PLAYBACK_STATUS_STOPPED;
            cid = a2dp_subevent_stream_released_get_a2dp_cid(packet);
            local_seid = a2dp_subevent_stream_released_get_local_seid(packet);
            
            printf("A2DP Source: Stream released, a2dp_cid 0x%02x, local_seid 0x%02x\n", cid, local_seid);

            if (cid == media_tracker.a2dp_cid) {
                media_tracker.stream_opened = 0;
                printf("A2DP Source: Stream released.\n");
            }
            if (media_tracker.avrcp_cid){
                // TODO: set now playing info
                // avrcp_target_set_now_playing_info(media_tracker.avrcp_cid, NULL, sizeof(tracks)/sizeof(avrcp_track_t));
                avrcp_target_set_playback_status(media_tracker.avrcp_cid, AVRCP_PLAYBACK_STATUS_STOPPED);
            }
            // TODO: stop timer - stop playing
            // a2dp_demo_timer_stop(&media_tracker);
            break;
        case A2DP_SUBEVENT_SIGNALING_CONNECTION_RELEASED:
            cid = a2dp_subevent_signaling_connection_released_get_a2dp_cid(packet);
            if (cid == media_tracker.a2dp_cid) {
                media_tracker.avrcp_cid = 0;
                media_tracker.a2dp_cid = 0;
                printf("A2DP Source: Signaling released.\n\n");
            }
            break;
        default:
            break; 
    }
}

static void avrcp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    bd_addr_t event_addr;
    uint16_t local_cid;
    uint8_t  status = ERROR_CODE_SUCCESS;

    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;
    
    switch (packet[2]){
        case AVRCP_SUBEVENT_CONNECTION_ESTABLISHED: 
            local_cid = avrcp_subevent_connection_established_get_avrcp_cid(packet);
            status = avrcp_subevent_connection_established_get_status(packet);
            if (status != ERROR_CODE_SUCCESS){
                printf("AVRCP: Connection failed, local cid 0x%02x, status 0x%02x\n", local_cid, status);
                return;
            }
            media_tracker.avrcp_cid = local_cid;
            avrcp_subevent_connection_established_get_bd_addr(packet, event_addr);

            printf("AVRCP: Channel to %s successfully opened, avrcp_cid 0x%02x\n", bd_addr_to_str(event_addr), media_tracker.avrcp_cid);
             
            avrcp_target_support_event(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_PLAYBACK_STATUS_CHANGED);
            avrcp_target_support_event(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_TRACK_CHANGED);
            avrcp_target_support_event(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_NOW_PLAYING_CONTENT_CHANGED);
            // TODO: Set now playing info
            // avrcp_target_set_now_playing_info(media_tracker.avrcp_cid, NULL, sizeof(tracks)/sizeof(avrcp_track_t));
            
            printf("Enable Volume Change notification\n");
            avrcp_controller_enable_notification(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_VOLUME_CHANGED);
            printf("Enable Battery Status Change notification\n");
            avrcp_controller_enable_notification(media_tracker.avrcp_cid, AVRCP_NOTIFICATION_EVENT_BATT_STATUS_CHANGED);
            return;
        
        case AVRCP_SUBEVENT_CONNECTION_RELEASED:
            printf("AVRCP Target: Disconnected, avrcp_cid 0x%02x\n", avrcp_subevent_connection_released_get_avrcp_cid(packet));
            media_tracker.avrcp_cid = 0;
            return;
        default:
            break;
    }

    if (status != ERROR_CODE_SUCCESS){
        printf("Responding to event 0x%02x failed, status 0x%02x\n", packet[2], status);
    }
}

static void avrcp_target_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    uint8_t  status = ERROR_CODE_SUCCESS;

    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;

    bool button_pressed;
    char const * button_state;
    avrcp_operation_id_t operation_id;

    switch (packet[2]){
        case AVRCP_SUBEVENT_PLAY_STATUS_QUERY:
            status = avrcp_target_play_status(media_tracker.avrcp_cid, play_info.song_length_ms, play_info.song_position_ms, play_info.status);            
            break;
        // case AVRCP_SUBEVENT_NOW_PLAYING_INFO_QUERY:
        //     status = avrcp_target_now_playing_info(avrcp_cid);
        //     break;
        case AVRCP_SUBEVENT_OPERATION:
            operation_id = avrcp_subevent_operation_get_operation_id(packet);
            button_pressed = avrcp_subevent_operation_get_button_pressed(packet) > 0;
            button_state = button_pressed ? "PRESS" : "RELEASE";

            printf("AVRCP Target: operation %s (%s)\n", avrcp_operation2str(operation_id), button_state);

            if (!button_pressed){
                break;
            }
            switch (operation_id) {
                case AVRCP_OPERATION_ID_PLAY:
                    status = a2dp_source_start_stream(media_tracker.a2dp_cid, media_tracker.local_seid);
                    break;
                case AVRCP_OPERATION_ID_PAUSE:
                    status = a2dp_source_pause_stream(media_tracker.a2dp_cid, media_tracker.local_seid);
                    break;
                case AVRCP_OPERATION_ID_STOP:
                    status = a2dp_source_disconnect(media_tracker.a2dp_cid);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    if (status != ERROR_CODE_SUCCESS){
        printf("Responding to event 0x%02x failed, status 0x%02x\n", packet[2], status);
    }
}

static void avrcp_controller_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    
    if (packet_type != HCI_EVENT_PACKET) return;
    if (hci_event_packet_get_type(packet) != HCI_EVENT_AVRCP_META) return;
    if (!media_tracker.avrcp_cid) return;
    
    switch (packet[2]){
        case AVRCP_SUBEVENT_NOTIFICATION_VOLUME_CHANGED:
            printf("AVRCP Controller: Notification Absolute Volume %d %%\n", avrcp_subevent_notification_volume_changed_get_absolute_volume(packet) * 100 / 127);
            break;
        case AVRCP_SUBEVENT_NOTIFICATION_EVENT_BATT_STATUS_CHANGED:
            // see avrcp_battery_status_t
            printf("AVRCP Controller: Notification Battery Status 0x%02x\n", avrcp_subevent_notification_event_batt_status_changed_get_battery_status(packet));
            break;
        case AVRCP_SUBEVENT_NOTIFICATION_STATE:
            printf("AVRCP Controller: Notification %s - %s\n", 
                avrcp_event2str(avrcp_subevent_notification_state_get_event_id(packet)), 
                avrcp_subevent_notification_state_get_enabled(packet) != 0 ? "enabled" : "disabled");
            break;
        default:
            break;
    }  
}

//// Audio producer

static const uint8_t NUM_CHANNELS = 2;

static int a2dp_fill_sbc_audio_buffer(a2dp_media_sending_context_t * context){
    // perform sbc encoding
    int total_num_bytes_read = 0;
    unsigned int num_audio_samples_per_sbc_buffer = btstack_sbc_encoder_num_audio_frames();
    while (context->samples_ready >= num_audio_samples_per_sbc_buffer
        && (context->max_media_payload_size - context->sbc_storage_count) >= btstack_sbc_encoder_sbc_buffer_length()){

        int16_t pcm_frame[1024*NUM_CHANNELS];

        blue_produce_audio(pcm_frame, num_audio_samples_per_sbc_buffer);
        btstack_sbc_encoder_process_data(pcm_frame);
        
        uint16_t sbc_frame_size = btstack_sbc_encoder_sbc_buffer_length(); 
        uint8_t * sbc_frame = btstack_sbc_encoder_sbc_buffer();
        
        total_num_bytes_read += num_audio_samples_per_sbc_buffer;
        // first byte in sbc storage contains sbc media header
        memcpy(&context->sbc_storage[1 + context->sbc_storage_count], sbc_frame, sbc_frame_size);
        context->sbc_storage_count += sbc_frame_size;
        context->samples_ready -= num_audio_samples_per_sbc_buffer;
    }
    return total_num_bytes_read;
}

static void a2dp_audio_timeout_handler(btstack_timer_source_t * timer){
    a2dp_media_sending_context_t * context = (a2dp_media_sending_context_t *) btstack_run_loop_get_timer_context(timer);
    btstack_run_loop_set_timer(&context->audio_timer, AUDIO_TIMEOUT_MS); 
    btstack_run_loop_add_timer(&context->audio_timer);
    uint32_t now = btstack_run_loop_get_time_ms();

    uint32_t update_period_ms = AUDIO_TIMEOUT_MS;
    if (context->time_audio_data_sent > 0){
        update_period_ms = now - context->time_audio_data_sent;
    } 

    uint32_t num_samples = (update_period_ms * current_sample_rate) / 1000;
    context->acc_num_missed_samples += (update_period_ms * current_sample_rate) % 1000;
    
    while (context->acc_num_missed_samples >= 1000){
        num_samples++;
        context->acc_num_missed_samples -= 1000;
    }
    context->time_audio_data_sent = now;
    context->samples_ready += num_samples;

    printf("a2dp_audio_timeout_handler num_samples %u, update_period_ms %u, samples missed %d\n", num_samples, update_period_ms, context->acc_num_missed_samples);

    if (context->sbc_ready_to_send) return;

    a2dp_fill_sbc_audio_buffer(context);

    if ((context->sbc_storage_count + btstack_sbc_encoder_sbc_buffer_length()) > context->max_media_payload_size){
        // schedule sending
        context->sbc_ready_to_send = 1;
        a2dp_source_stream_endpoint_request_can_send_now(context->a2dp_cid, context->local_seid);
    }
}

static void a2dp_timer_start(a2dp_media_sending_context_t * context){
    context->max_media_payload_size = btstack_min(a2dp_max_media_payload_size(context->a2dp_cid, context->local_seid), SBC_STORAGE_SIZE);
    context->sbc_storage_count = 0;
    context->sbc_ready_to_send = 0;
    context->streaming = 1;
    btstack_run_loop_remove_timer(&context->audio_timer);
    btstack_run_loop_set_timer_handler(&context->audio_timer, a2dp_audio_timeout_handler);
    btstack_run_loop_set_timer_context(&context->audio_timer, context);
    btstack_run_loop_set_timer(&context->audio_timer, AUDIO_TIMEOUT_MS); 
    btstack_run_loop_add_timer(&context->audio_timer);
}

static void a2dp_demo_send_media_packet(void){
    int num_bytes_in_frame = btstack_sbc_encoder_sbc_buffer_length();
    int bytes_in_storage = media_tracker.sbc_storage_count;
    uint8_t num_sbc_frames = bytes_in_storage / num_bytes_in_frame;
    // Prepend SBC Header
    media_tracker.sbc_storage[0] = num_sbc_frames;  // (fragmentation << 7) | (starting_packet << 6) | (last_packet << 5) | num_frames;
    a2dp_source_stream_send_media_payload_rtp(media_tracker.a2dp_cid, media_tracker.local_seid, 0,
                                               media_tracker.rtp_timestamp,
                                               media_tracker.sbc_storage, bytes_in_storage + 1);

    // update rtp_timestamp
    unsigned int num_audio_samples_per_sbc_buffer = btstack_sbc_encoder_num_audio_frames();
    media_tracker.rtp_timestamp += num_sbc_frames * num_audio_samples_per_sbc_buffer;

    media_tracker.sbc_storage_count = 0;
    media_tracker.sbc_ready_to_send = 0;
}