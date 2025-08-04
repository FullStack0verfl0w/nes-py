//  Program:      nes-py
//  File:         mapper_CNROM.cpp
//  Description:  An implementation of the CNROM mapper
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "mappers/mapper_CNROM.hpp"
#include "log.hpp"

namespace NES {

void MapperCNROM::save_state(StateWriter& w) const {
    w.write(select_chr);
}
void MapperCNROM::load_state(StateReader& r) {
    r.read(select_chr);
    r.skip_remainder();
}

void MapperCNROM::writeCHR(NES_Address address, NES_Byte value) {
    LOG(Info) <<
        "Read-only CHR memory write attempt at " <<
        std::hex <<
        address <<
        std::endl;
}

}  // namespace NES
