#include "csvtable.h"

#include <cstring>

CsvTable::CsvTable(ReadCallback read_callback)
    : opened_(false),
      read_callback_(read_callback)
{
}

bool CsvTable::open(void *file_context)
{
    opened_ = false;

    file_context_ = file_context;

    while (true) {
        if (!parse_line()) {
            return false;
        }

        if (line_[0] != '#') {
            break;
        }
    }

    header_field_count_ = 0;

    while (header_field_count_ < field_count_) {
        memset(header_fields_[header_field_count_],
               0,
               MAX_HEADER_FIELD_LENGTH + 1);
        strncpy(header_fields_[header_field_count_],
                fields_[header_field_count_],
                MAX_HEADER_FIELD_LENGTH);
        header_field_count_++;
    }

    opened_ = true;

    return true;
}

void CsvTable::close()
{
    opened_ = false;
}

bool CsvTable::parse_line()
{
    size_t length = 0;
    bool valid = true;

    while (length < MAX_LINE_LENGTH) {
        uint8_t character;

        if (read(&character, 1) < 1) {
            valid = false;
            break;
        }

        if (character == '\n') {
            break;
        }

        if (character != '\r') {
            line_[length] = static_cast<char>(character);
            length++;
        }
    }

    line_[length] = '\0';

    field_count_ = 0;

    if (valid) {
        char *handle = line_;

        while (field_count_ < MAX_FIELD_COUNT) {
            char *field = strtok(handle, ",");
            handle = nullptr;
            if (!field) {
                break;
            }

            fields_[field_count_] = field;
            field_count_++;
        }
    }

    return valid;
}

const char *CsvTable::field(size_t index)
{
    if (!opened_) {
        return nullptr;
    }

    if (index >= field_count_) {
        return nullptr;
    }

    return fields_[index];
}

const char *CsvTable::field(const char *header)
{
    if (!opened_) {
        return nullptr;
    }

    size_t index;

    for (index = 0; index < header_field_count_; index++) {
        if (strcmp(header, header_fields_[index]) == 0) {
            break;
        }
    }

    if (index == header_field_count_) {
        return nullptr;
    }

    return field(index);
}

inline size_t CsvTable::read(uint8_t *buffer, size_t length)
{
    return read_callback_(file_context_, buffer, length);
}
