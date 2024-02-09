#include "parser.h"
#include <cassert>
#include <cstdio>

using namespace sdemo;

uint32_t filereader_t::read(void* dest, uint32_t bytes)
{
    auto result = fread(dest, 1, bytes, m_pFile);

    if(result < bytes)
    {
        memset((uint8_t*)dest + result, 0, bytes - result);
    }

    return result;
}

bool filereader_t::eof()
{
    return feof(m_pFile) != 0;
}

bool filereader_t::try_open(const char* filepath)
{
    m_pFile = fopen(filepath, "rb");
    return m_pFile != nullptr;
}

void filereader_t::close()
{
    fclose(m_pFile);
}

sdemo::error_code parser_t::parse(ireader_t* reader, demo_t* output)
{
    parser_t parser;
    parser.m_pOutput = output;
    parser.m_pReader = reader;
    parser.parse_header();

    auto result = sdemo::error_code::success;

    while(result == sdemo::error_code::success) {
        result = parser.parse_anymessage();
    }

    if(result == sdemo::error_code::eof) {
        result = sdemo::error_code::success;
    }

    return result;
}

void parser_t::parse_header()
{
    auto* header = &m_pOutput->m_header;

    m_pReader->read(&header->ID);
    m_pReader->read(&header->demo_protocol);
    m_pReader->read(&header->net_protocol);
    m_pReader->read(&header->server_name);
    m_pReader->read(&header->client_name);
    m_pReader->read(&header->map_name);
    m_pReader->read(&header->game_directory);
    m_pReader->read(&header->seconds);
    m_pReader->read(&header->tick_count);
    m_pReader->read(&header->frame_count);
    m_pReader->read(&header->signon_length);

    header->ID[7] = header->server_name[259] = header->client_name[259] = header->map_name[259] = header->game_directory[259] = '\0';

    m_pOutput->m_version = sdemo::get_demo_version(header);
}

sdemo::error_code parser_t::parse_anymessage()
{
    std::uint8_t type = m_pReader->read_value<std::uint8_t>();

    switch(type) {
        case (int)messagetype_t::consolecmd:
            return parse_consolecmd();
        case (int)messagetype_t::datatables:
            return parse_datatables();
        case (int)messagetype_t::packet:
            return parse_packet((messagetype_t)type);
        case (int)messagetype_t::signon:
            return parse_packet((messagetype_t)type);
        case (int)messagetype_t::stop:
            return parse_stop();
        case (int)messagetype_t::synctick:
            return parse_synctick();
        case (int)messagetype_t::usercmd:
            return parse_usercmd();
        case 8:
            if(m_pOutput->m_version.demo_protocol < 4) {
                return parse_stringtables();
            } else {
                return parse_customdata();
            }
        case 9:
            return parse_stringtables();
        default:
            return sdemo::error_code::bad_message;
    }
}

static void parse_preamble(parser_t* parser, sdemo::message_preamble* preamble) {
    preamble->tick = parser->m_pReader->read_value<std::int32_t>();
    if(parser->m_pOutput->m_version.has_slot_in_preamble) {
        preamble->slot = parser->m_pReader->read_value<std::uint8_t>(); 
    }
}

template<typename T> sdemo::error_code _parse_data(parser_t* parser, T& message)
{
    parser->m_pReader->read(&message.size_bytes);

    if(message.size_bytes > 0) {
        message.data = (char*)malloc(message.size_bytes);
        if(message.data != nullptr) {
            parser->m_pReader->read(message.data, message.size_bytes);
        } else {
            return sdemo::error_code::invalid_data;
        }
    } else if (message.size_bytes == 0) {
        message.data = nullptr;
    } else {
        return sdemo::error_code::invalid_data;
    }

    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_consolecmd() {
    sdemo::consolecmd_t message;
    parse_preamble(this, &message.preamble);
    auto result = _parse_data(this, message);

    if(result != sdemo::error_code::success) {
        return result;
    }

    message.data[message.size_bytes - 1] = '\0';
    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_datatables() {
    sdemo::datatables_t message;
    parse_preamble(this, &message.preamble);
    auto result = _parse_data(this, message);

    if(result != sdemo::error_code::success) {
        return result;
    }

    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_packet(messagetype_t type) {
    sdemo::packet_t packet;
    parse_preamble(this, &packet.preamble);

    for(size_t i=0; i < m_pOutput->m_version.cmdinfo_size; ++i) {
        m_pReader->read(&packet.cmdinfo[i].interp_flags);
        m_pReader->read(&packet.cmdinfo[i].view_origin);
        m_pReader->read(&packet.cmdinfo[i].view_angles);
        m_pReader->read(&packet.cmdinfo[i].local_viewangles);
        m_pReader->read(&packet.cmdinfo[i].view_origin2);
        m_pReader->read(&packet.cmdinfo[i].view_angles2);
        m_pReader->read(&packet.cmdinfo[i].local_viewangles2);
    }

    m_pReader->read(&packet.in_sequence);
    m_pReader->read(&packet.out_sequence);
    m_pReader->read(&packet.size_bytes);

    if(packet.size_bytes > 0) {
        packet.data = malloc(packet.size_bytes);
        if(packet.data != nullptr) {
            m_pReader->read(packet.data, packet.size_bytes);
        } else {
            return sdemo::error_code::invalid_data;
        }
    } else if (packet.size_bytes == 0) {
        packet.data = nullptr;
    } else {
        return sdemo::error_code::invalid_data;
    }

    m_pOutput->m_vec_messages.push_back(std::move(packet));

    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_stop() {
    stop_t message;
    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::eof;
}

sdemo::error_code sdemo::parser_t::parse_synctick() {
    synctick_t message;
    parse_preamble(this, &message.preamble);
    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_usercmd() {
    usercmd_t message;
    parse_preamble(this, &message.preamble);
    m_pReader->read(&message.user_cmd);
    auto result = _parse_data(this, message);

    if(result != sdemo::error_code::success) {
        return result;
    }

    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_stringtables() {
    stringtables_t message;
    parse_preamble(this, &message.preamble);
    auto result = _parse_data(this, message);

    if(result != sdemo::error_code::success) {
        return result;
    }

    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::success;
}

sdemo::error_code sdemo::parser_t::parse_customdata() {
    customdata_t message;
    parse_preamble(this, &message.preamble);
    auto result = _parse_data(this, message);

    if(result != sdemo::error_code::success) {
        return result;
    }

    m_pOutput->m_vec_messages.push_back(std::move(message));
    return sdemo::error_code::success;
}

sdemo::error_code demo_t::parse_from(const char* filepath, demo_t* output)
{
    filereader_t reader;
    if(!reader.try_open(filepath)) {
        return sdemo::error_code::no_such_file_or_directory;
    }

    parser_t parser;
    return parser.parse(&reader, output);
}

demomessage_t sdemo::clone_message(const demomessage_t* msg)
{
    return std::visit([](auto& arg) -> demomessage_t { return arg.clone(); }, *msg);
}
    
void sdemo::free_message(demomessage_t* msg)
{
    return std::visit([](auto& arg) { arg.destroy(); }, *msg);
}

demo_t::~demo_t()
{
    for(demomessage_t& msg : m_vec_messages)
        std::visit([](auto& arg) { arg.destroy(); }, msg);
}

template<typename T>
static demomessage_t clone_generic(const T* ptr) {
    T cmd = *ptr;
    cmd.data = malloc(cmd.size_bytes);
    memcpy(cmd.data, ptr->data, cmd.size_bytes);
    return cmd;
}

demomessage_t consolecmd_t::clone() const {
    consolecmd_t cmd = *this;
    cmd.data = (char*)malloc(cmd.size_bytes);
    memcpy(cmd.data, this->data, this->size_bytes);
    return cmd;
}

demomessage_t customdata_t::clone() const {
    return clone_generic(this);
}

demomessage_t datatables_t::clone() const {
    return clone_generic(this);
}

demomessage_t packet_t::clone() const {
    return clone_generic(this);
}

demomessage_t stop_t::clone() const {
    return clone_generic(this);
}

demomessage_t stringtables_t::clone() const {
    return clone_generic(this);
}

demomessage_t synctick_t::clone() const {
    return *this;
}

demomessage_t usercmd_t::clone() const {
    return clone_generic(this);
}

void consolecmd_t::destroy() {
    free(data);
}

void customdata_t::destroy() {
    free(data);
}

void datatables_t::destroy() {
    free(data);
}

void packet_t::destroy() {
    free(data);
}

void stop_t::destroy() {
    free(data);
}

void stringtables_t::destroy() {
    free(data);
}

void synctick_t::destroy() { }

void usercmd_t::destroy() {
    free(data);
}

