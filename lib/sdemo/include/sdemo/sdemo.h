#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <system_error>
#include <variant>
#include <vector>

namespace sdemo
{
    enum class error_code { success = 0, eof, invalid_data, bad_message, no_such_file_or_directory };

    #define sdemo_MACRO_ALL_MESSAGES(macro) \
        macro(net_nop)  \
        macro(net_disconnect) \
        macro(net_file) \
        macro(net_tick) \
        macro(net_stringcmd) \
        macro(net_setconvar) \
        macro(net_signonstate) \
        macro(net_splitscreen_user) \
        macro(svc_print) \
        macro(svc_serverinfo) \
        macro(svc_sendtable) \
        macro(svc_classinfo) \
        macro(svc_setpause) \
        macro(svc_create_stringtable) \
        macro(svc_update_stringtable) \
        macro(svc_voice_init) \
        macro(svc_voice_data) \
        macro(svc_sounds) \
        macro(svc_setview) \
        macro(svc_fixangle) \
        macro(svc_crosshair_angle) \
        macro(svc_bsp_decal) \
        macro(svc_user_message) \
        macro(svc_entity_message) \
        macro(svc_game_event) \
        macro(svc_packet_entities) \
        macro(svc_splitscreen) \
        macro(svc_temp_entities) \
        macro(svc_prefetch) \
        macro(svc_menu) \
        macro(svc_game_event_list) \
        macro(svc_get_cvar_value) \
        macro(svc_cmd_key_values) \
        macro(svc_paintmap_data)

    #define sdemo_DECLARE_ENUM(x) x,

    enum class net_message_t { sdemo_MACRO_ALL_MESSAGES(sdemo_DECLARE_ENUM) svc_invalid };

    enum class game_t { csgo, l4d, l4d2, portal2, orangebox, steampipe };

    struct version_t {
        enum game_t game : 3;
        unsigned int netmessage_type_bits : 3;
        unsigned int demo_protocol : 3;
        unsigned int cmdinfo_size : 3;
        unsigned int net_file_bits : 2;
        unsigned int stringtable_flags_bits : 2;
        unsigned int stringtable_userdata_size_bits : 5;
        unsigned int svc_user_message_bits : 4;
        unsigned int svc_prefetch_bits : 4;
        unsigned int model_index_bits : 4;
        unsigned int datatable_propcount_bits : 4;
        unsigned int sendprop_flag_bits : 5;
        unsigned int sendprop_numbits_for_numbits : 4;
        unsigned int has_slot_in_preamble : 1;
        unsigned int has_nettick_times : 1;
        unsigned int l4d2_version_finalized : 1;
        unsigned int svc_update_stringtable_table_id_bits : 4;
        net_message_t *netmessage_array = nullptr;
        unsigned int netmessage_count = 0;
        unsigned int network_protocol = 0;
        unsigned int l4d2_version = 0;
    };

    enum class messagetype_t {
        signon = 1,
        packet = 2,
        synctick = 3,
        consolecmd = 4,
        usercmd = 5,
        datatables = 6,
        stop = 7,
        customdata = 8,
        stringtables = 9
        // These arent necessarily 1 : 1 with the number in the demo
        // Customdata is 8 and Stringtables is either 8 or 9 depending on version
    };

    struct header_t {
        char ID[8];
        int32_t demo_protocol;
        int32_t net_protocol;
        char server_name[260];
        char client_name[260];
        char map_name[260];
        char game_directory[260];
        float seconds;
        int32_t tick_count;
        int32_t frame_count;
        int32_t signon_length;
    };

    struct vector {
        float x, y, z;
    };

    struct message_preamble {
        int32_t tick;
        uint8_t slot;
    };

    struct cmdinfo_t {
        int32_t interp_flags;
        vector view_origin;
        vector view_angles;
        vector local_viewangles;
        vector view_origin2;
        vector view_angles2;
        vector local_viewangles2;
    };

    struct consolecmd_t;
    struct customdata_t;
    struct datatables_t;
    struct packet_t;
    struct stop_t;
    struct stringtables_t;
    struct synctick_t;
    struct usercmd_t;

    typedef std::variant<consolecmd_t, customdata_t, datatables_t, packet_t, stop_t, stringtables_t, synctick_t, usercmd_t> demomessage_t;

    struct customdata_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
        int32_t size_bytes = 0;
        void* data = nullptr;
    };

    struct consolecmd_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
        int32_t size_bytes = 0;
        char* data = nullptr;
    };

    struct datatables_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
        int32_t size_bytes = 0;
        void* data = nullptr;
    };

    struct packet_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
        cmdinfo_t cmdinfo[4]; // Some games have multiple cmdinfos, split screen
        size_t cmdinfo_size = 0;
        int32_t in_sequence = 0;
        int32_t out_sequence = 0;
        int32_t size_bytes = 0;
        void* data = nullptr;
    };

    struct stop_t {
        demomessage_t clone() const;
        void destroy();
        int32_t size_bytes = 0;
        void* data = nullptr;
    };

    struct stringtables_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
        int32_t size_bytes = 0;
        void* data = nullptr;
    };

    struct synctick_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
    };

    struct usercmd_t {
        demomessage_t clone() const;
        void destroy();
        message_preamble preamble;
        int32_t user_cmd = 0;
        int32_t size_bytes = 0;
        void* data = nullptr;
    };

    demomessage_t clone_message(const demomessage_t* msg);
    void free_message(demomessage_t* msg);

    struct demo_t
    {
        header_t m_header;
        version_t m_version;
        std::vector<demomessage_t> m_vec_messages;
        demo_t(const demo_t& d) = delete;
        demo_t() = default;
        ~demo_t();
        static sdemo::error_code parse_from(const char* filepath, demo_t* output);
    };

    // Version utils
    version_t get_demo_version(sdemo::header_t *header);
    void version_update_build_info(sdemo::version_t *version);
    bool get_l4d2_build(const char *str, int *out);
}
