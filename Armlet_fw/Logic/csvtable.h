#pragma once

#include <cstddef>
#include <cstdint>

class CsvTable
{
public:
    typedef size_t (*ReadCallback)(void *file_context, uint8_t *buffer, size_t length);

    static const size_t MAX_LINE_LENGTH = 255;

    static const size_t MAX_FIELD_COUNT = 12;
    static const size_t MAX_HEADER_FIELD_LENGTH = 15;

public:
    CsvTable(ReadCallback read_callback);

    bool open(void *file_context);

    void close();

    bool opened()
    {
        return opened_;
    }

    bool parse_line();

    const char *field(size_t index);
    const char *field(const char *header);

private:
    inline size_t tell();
    inline bool seek(size_t offset);
    inline size_t read(uint8_t *buffer, size_t length);

private:
    bool opened_;

    void *file_context_;

    ReadCallback read_callback_;

    char line_[MAX_LINE_LENGTH + 1];
    char *fields_[MAX_FIELD_COUNT];
    size_t field_count_;

    char header_fields_[MAX_FIELD_COUNT][MAX_HEADER_FIELD_LENGTH + 1];
    size_t header_field_count_;
};
