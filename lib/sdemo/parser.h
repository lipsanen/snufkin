#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <system_error>
#include "sdemo/sdemo.h"

namespace sdemo
{
    class ireader_t
    {
    public:
        virtual ~ireader_t() {}

        template<typename T>
        void read(T* dest)
        {
            read(dest, sizeof(T));
        }

        template<typename T>
        T read_value() { T out; read(&out); return out; }
        virtual uint32_t read(void* dest, uint32_t bytes) = 0;
        virtual bool eof() = 0;
        bool m_bEOF = false;
    };

    class filereader_t : public ireader_t
    {
    public:
        bool try_open(const char* filepath);
        void close();
        FILE* m_pFile = nullptr;
        virtual uint32_t read(void* dest, uint32_t bytes) override;
        virtual bool eof() override;
    };

    struct parser_t
    {
        static sdemo::error_code parse(ireader_t* reader, demo_t* output);
        void parse_header();
        sdemo::error_code parse_anymessage();
        sdemo::error_code parse_consolecmd();
        sdemo::error_code parse_datatables();
        sdemo::error_code parse_packet(messagetype_t type);
        sdemo::error_code parse_stop();
        sdemo::error_code parse_synctick();
        sdemo::error_code parse_usercmd();
        sdemo::error_code parse_stringtables();
        sdemo::error_code parse_customdata();

        ireader_t* m_pReader = nullptr;
        demo_t* m_pOutput = nullptr;
    };
}
