#include "decoder.h"

using namespace frsky;

Decoder::Decoder()
:_idx(0)
,_escape(false)
,_voltage_frame_counter(0)
{

}

Decoder::DECODE_STATE Decoder::process_byte(uint8_t incoming_octet)
{
    // Start with start/stop marker, out-of-sync therwize -> reset
    if ((_idx == 0) && (incoming_octet != START_STOP_MARKER)) {
        reset();
        return OUT_OF_SYNC;
    }

    // Ends with Start with start/stop marker, out-of-sync otherwise -> reset
    if ((_idx == sizeof(_buffer) - 1)) {
        if (incoming_octet == START_STOP_MARKER) {
            // end of frame
             _buffer[_idx] = START_STOP_MARKER;
             reset();

             // TODO decode
             return decode();
        } else {
            reset();
            return OUT_OF_SYNC;
        }
    }

    // Escape
    if (_escape) {
        // Process escape
        _escape = false;
        incoming_octet ^= ESCAPE_XOR_MASK;
        _buffer[_idx++] = incoming_octet;
        return NEED_MORE_BYTES;
    } else if (incoming_octet == ESCAPE_MARKER) {
        // Start escape
        _escape = true;
        return NEED_MORE_BYTES;
    }

    if ((_idx > 0) && (incoming_octet == START_STOP_MARKER)) {
        // Start/stop marker in the middle of frame -> some bytes are lost and new frame is started here
        reset();
        _buffer[_idx++] = incoming_octet;
        return OUT_OF_SYNC;
    } else {
        _buffer[_idx++] = incoming_octet;
        return NEED_MORE_BYTES;
    }

}

Decoder::DECODE_STATE Decoder::decode()
{
    if (_buffer[1] == 0xFE) {
        // Voltage and RSSI/TSSI

        _xssi.a1 = _buffer[2];
        _xssi.a2 = _buffer[3];
        _xssi.rssi = _buffer[4];
        _xssi.tssi = _buffer[5];

        _voltage_frame_counter++;
        return FRAME_COMPLETE_VOLTAGE_RSSI;
    } else {
        return FRAME_COMPLETE;
    }
}

