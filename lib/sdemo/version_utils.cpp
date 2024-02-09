#include "sdemo/sdemo.h"
#include "sdemo/utils.h"
#include <string.h>

// clang-format off
// protocol 2
static sdemo::net_message_t protocol_2_messages[] =
{
  sdemo::net_message_t::svc_invalid,
  sdemo::net_message_t::net_nop,
  sdemo::net_message_t::net_disconnect,
  sdemo::net_message_t::svc_invalid, // svc_event
  sdemo::net_message_t::svc_invalid, // svc_version
  sdemo::net_message_t::svc_setview, 
  sdemo::net_message_t::svc_sounds,
  sdemo::net_message_t::svc_invalid, // svc_time
  sdemo::net_message_t::svc_print,
  sdemo::net_message_t::svc_invalid, // svc_stufftext
  sdemo::net_message_t::svc_crosshair_angle, // svc_setangle
  sdemo::net_message_t::svc_serverinfo,
  sdemo::net_message_t::svc_invalid, // svc_lightstyles
  sdemo::net_message_t::svc_invalid, // svc_updateuserinfo
  sdemo::net_message_t::svc_invalid, // svc_event_reliable
  sdemo::net_message_t::svc_invalid, // svc_clientdata
  sdemo::net_message_t::svc_invalid, // svc_stopsound
  sdemo::net_message_t::svc_create_stringtable,
  sdemo::net_message_t::svc_update_stringtable,
  sdemo::net_message_t::svc_entity_message,
  sdemo::net_message_t::svc_invalid, // svc_spawnbaseline
  sdemo::net_message_t::svc_bsp_decal, 
  sdemo::net_message_t::svc_setpause,
  sdemo::net_message_t::svc_invalid, // svc_signonnum
  sdemo::net_message_t::svc_invalid, // svc_centerprint
  sdemo::net_message_t::svc_invalid, // svc_spawnstaticsound
  sdemo::net_message_t::svc_game_event
};

// steampipe and orangebox
static sdemo::net_message_t old_protocol_messages[] =
{
  sdemo::net_message_t::net_nop,
  sdemo::net_message_t::net_disconnect,
  sdemo::net_message_t::net_file,
  sdemo::net_message_t::net_tick,
  sdemo::net_message_t::net_stringcmd,
  sdemo::net_message_t::net_setconvar,
  sdemo::net_message_t::net_signonstate,
  sdemo::net_message_t::svc_print,
  sdemo::net_message_t::svc_serverinfo,
  sdemo::net_message_t::svc_sendtable,
  sdemo::net_message_t::svc_classinfo,
  sdemo::net_message_t::svc_setpause,
  sdemo::net_message_t::svc_create_stringtable,
  sdemo::net_message_t::svc_update_stringtable,
  sdemo::net_message_t::svc_voice_init,
  sdemo::net_message_t::svc_voice_data,
  sdemo::net_message_t::svc_invalid, // svc_HLTV
  sdemo::net_message_t::svc_sounds,
  sdemo::net_message_t::svc_setview,
  sdemo::net_message_t::svc_fixangle,
  sdemo::net_message_t::svc_crosshair_angle,
  sdemo::net_message_t::svc_bsp_decal,
  sdemo::net_message_t::svc_invalid, // svc_terrainmod
  sdemo::net_message_t::svc_user_message,
  sdemo::net_message_t::svc_entity_message,
  sdemo::net_message_t::svc_game_event,
  sdemo::net_message_t::svc_packet_entities,
  sdemo::net_message_t::svc_temp_entities,
  sdemo::net_message_t::svc_prefetch,
  sdemo::net_message_t::svc_menu,
  sdemo::net_message_t::svc_game_event_list,
  sdemo::net_message_t::svc_get_cvar_value,
  sdemo::net_message_t::svc_cmd_key_values // steampipe only
};

static sdemo::net_message_t new_protocol_messages[] =
{
  sdemo::net_message_t::net_nop,
  sdemo::net_message_t::net_disconnect,
  sdemo::net_message_t::net_file,
  sdemo::net_message_t::net_splitscreen_user,
  sdemo::net_message_t::net_tick,
  sdemo::net_message_t::net_stringcmd,
  sdemo::net_message_t::net_setconvar,
  sdemo::net_message_t::net_signonstate,
  sdemo::net_message_t::svc_serverinfo,
  sdemo::net_message_t::svc_sendtable,
  sdemo::net_message_t::svc_classinfo,
  sdemo::net_message_t::svc_setpause,
  sdemo::net_message_t::svc_create_stringtable,
  sdemo::net_message_t::svc_update_stringtable,
  sdemo::net_message_t::svc_voice_init,
  sdemo::net_message_t::svc_voice_data,
  sdemo::net_message_t::svc_print,
  sdemo::net_message_t::svc_sounds,
  sdemo::net_message_t::svc_setview,
  sdemo::net_message_t::svc_fixangle,
  sdemo::net_message_t::svc_crosshair_angle,
  sdemo::net_message_t::svc_bsp_decal,
  sdemo::net_message_t::svc_splitscreen,
  sdemo::net_message_t::svc_user_message,
  sdemo::net_message_t::svc_entity_message,
  sdemo::net_message_t::svc_game_event,
  sdemo::net_message_t::svc_packet_entities,
  sdemo::net_message_t::svc_temp_entities,
  sdemo::net_message_t::svc_prefetch,
  sdemo::net_message_t::svc_menu,
  sdemo::net_message_t::svc_game_event_list,
  sdemo::net_message_t::svc_get_cvar_value,
  sdemo::net_message_t::svc_cmd_key_values, // Not in old L4D
  sdemo::net_message_t::svc_paintmap_data // Added in protocol 4
};
// clang-format on

static bool is_oe(sdemo::version_t *version) { return version->network_protocol <= 7; }

static void get_net_message_array(sdemo::version_t *version) {
  if (version->demo_protocol == 1) {
    version->netmessage_array = protocol_2_messages;
    version->netmessage_count = ARRAYSIZE(protocol_2_messages);
  } else if (version->demo_protocol <= 3) {
    version->netmessage_array = old_protocol_messages;
    version->netmessage_count = ARRAYSIZE(old_protocol_messages);
  } else {
    version->netmessage_array = new_protocol_messages;
    version->netmessage_count = ARRAYSIZE(new_protocol_messages);
  }
}

static void get_net_message_bits(sdemo::version_t *version) {
  if (version->network_protocol <= 14) {
    version->netmessage_type_bits = 5;
  } else if (version->game == sdemo::game_t::l4d && version->network_protocol <= 37) {
    version->netmessage_type_bits = 5;
  } else {
    version->netmessage_type_bits = 6;
  }
}

static void get_preamble_info(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::portal2 || version->game == sdemo::game_t::csgo || version->game == sdemo::game_t::l4d ||
      version->game == sdemo::game_t::l4d2) {
    version->has_slot_in_preamble = true;
  } else {
    version->has_slot_in_preamble = false;
  }
}

static void get_cmdinfo_size(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::l4d || version->game == sdemo::game_t::l4d2) {
    version->cmdinfo_size = 4;
  } else if (version->game == sdemo::game_t::portal2 || version->game == sdemo::game_t::csgo) {
    version->cmdinfo_size = 2;
  } else {
    version->cmdinfo_size = 1;
  }
}

static void get_net_file_bits(sdemo::version_t *version) {
  if (version->demo_protocol <= 3) {
    version->net_file_bits = 1;
  } else {
    version->net_file_bits = 2;
  }
}

static void get_has_nettick_times(sdemo::version_t *version) {
  if (version->network_protocol < 11) {
    version->has_nettick_times = false;
  } else {
    version->has_nettick_times = true;
  }
}

static void get_stringtable_flags_bits(sdemo::version_t *version) {
  if (version->demo_protocol >= 4) {
    version->stringtable_flags_bits = 2;
  } else {
    version->stringtable_flags_bits = 1;
  }
}

static void get_stringtable_userdata_size_bits(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::l4d2) {
    version->stringtable_userdata_size_bits = 21;
  } else {
    version->stringtable_userdata_size_bits = 20;
  }
}

static void get_svc_user_message_bits(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::l4d || version->game == sdemo::game_t::l4d2) {
    version->svc_user_message_bits = 11;
  } else if (version->demo_protocol >= 4) {
    version->svc_user_message_bits = 12;
  } else {
    version->svc_user_message_bits = 11;
  }
}

static void get_svc_prefetch_bits(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::l4d2 && version->l4d2_version >= 2091) {
    version->svc_prefetch_bits = 15;
  } else if (version->game == sdemo::game_t::l4d2 || version->game == sdemo::game_t::steampipe) {
    version->svc_prefetch_bits = 14;
  } else {
    version->svc_prefetch_bits = 13;
  }
}

static void get_model_index_bits(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::l4d2 && version->l4d2_version >= 2203) {
    version->model_index_bits = 12;
  } else if (version->game == sdemo::game_t::steampipe) {
    version->model_index_bits = 13;
  } else {
    version->model_index_bits = 11;
  }
}

static void get_datatable_propcount_bits(sdemo::version_t *version) {
  if (is_oe(version)) {
    version->datatable_propcount_bits = 9;
  } else {
    version->datatable_propcount_bits = 10;
  }
}

static void get_sendprop_flag_bits(sdemo::version_t *version) {
  if (version->demo_protocol == 2) {
    version->sendprop_flag_bits = 11;
  } else if (is_oe(version)) {
    version->sendprop_flag_bits = 13;
  } else if (version->game == sdemo::game_t::l4d || version->demo_protocol == 3) {
    version->sendprop_flag_bits = 16;
  } else {
    version->sendprop_flag_bits = 19;
  }
}

static void get_sendprop_numbits_for_numbits(sdemo::version_t *version) {
  if (version->game == sdemo::game_t::l4d || version->game == sdemo::game_t::l4d2 || version->network_protocol <= 14) {
    version->sendprop_numbits_for_numbits = 6;
  } else {
    version->sendprop_numbits_for_numbits = 7;
  }
}

static void get_svc_update_stringtable_table_id_bits(sdemo::version_t *version) {
  if (version->network_protocol < 11) {
    version->svc_update_stringtable_table_id_bits = 4;
  } else {
    version->svc_update_stringtable_table_id_bits = 5;
  }
}

struct version_pair {
  const char *game_directory;
  sdemo::game_t version;
};

static version_pair versions[] = {{"aperturetag", sdemo::game_t::portal2},    {"portal2", sdemo::game_t::portal2},
                                  {"portalreloaded", sdemo::game_t::portal2}, {"portal_stories", sdemo::game_t::portal2},
                                  {"TWTM", sdemo::game_t::portal2},           {"csgo", sdemo::game_t::csgo},
                                  {"left4dead2", sdemo::game_t::l4d2},        {"left4dead", sdemo::game_t::l4d}};

sdemo::version_t sdemo::get_demo_version(sdemo::header_t *header) {
  sdemo::version_t version;
  version.game = sdemo::game_t::orangebox;
  version.demo_protocol = header->demo_protocol;
  version.network_protocol = header->net_protocol;

  if (version.demo_protocol <= 3) {
    if (version.network_protocol >= 24) {
      version.game = sdemo::game_t::steampipe;
    } else {
      version.game = sdemo::game_t::orangebox;
    }
  } else {
    for (size_t i = 0; i < ARRAYSIZE(versions); ++i) {
      if (strcmp(header->game_directory, versions[i].game_directory) == 0) {
        version.game = versions[i].version;
        break;
      }
    }
  }

  version.l4d2_version_finalized = true;

  if (version.game == sdemo::game_t::l4d2) {
    if (header->net_protocol >= 2100) {
      version.l4d2_version = 2203;
    } else if (header->net_protocol == 2042) {
      // protocol 2042 is resolved in parsing svc_print
      version.l4d2_version_finalized = false;
      version.l4d2_version = 2042;
    } else {
      version.l4d2_version = header->net_protocol;
    }
  } else {
    version.l4d2_version = 0;
  }

  sdemo::version_update_build_info(&version);

  return version;
}

void sdemo::version_update_build_info(sdemo::version_t *version) {
  // This can be re-run in the case of l4d2 demos
  get_cmdinfo_size(version);
  get_preamble_info(version);
  get_net_message_array(version);
  get_net_message_bits(version);
  get_net_file_bits(version);
  get_has_nettick_times(version);
  get_stringtable_flags_bits(version);
  get_stringtable_userdata_size_bits(version);
  get_svc_user_message_bits(version);
  get_svc_prefetch_bits(version);
  get_model_index_bits(version);
  get_datatable_propcount_bits(version);
  get_sendprop_flag_bits(version);
  get_sendprop_numbits_for_numbits(version);
  get_svc_update_stringtable_table_id_bits(version);
}

bool sdemo::get_l4d2_build(const char *str, int *out) {
  const char l4d2_buildstring_start[] = "\nLeft 4 Dead 2\nMap";
  int length_without_null = ARRAYSIZE(l4d2_buildstring_start) - 1;
  for (int i = 0; i < length_without_null; ++i) {
    if (str[i] != l4d2_buildstring_start[i]) {
      return false;
    }
  }

  const char build_string[] = "Build:";
  const char *build_loc = strstr(str, build_string);

  if (build_loc) {
    const char *build_num = build_loc + ARRAYSIZE(build_string);
    *out = atoi(build_num);

    return true;
  } else {
    return false;
  }
}
