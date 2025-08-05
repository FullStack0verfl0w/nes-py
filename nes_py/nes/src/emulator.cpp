//  Program:      nes-py
//  File:         emulator.cpp
//  Description:  This class houses the logic and data for an NES emulator
//
//  Copyright (c) 2019 Christian Kauten. All rights reserved.
//

#include "cstring"
#include "emulator.hpp"
#include "mapper_factory.hpp"
#include "log.hpp"
#include "state.hpp"

#include <fstream>
#include <stdexcept>

namespace NES {


void Emulator::save_state(std::string filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Emulator::save_state: cannot open '" + filename + "' for writing");

    file.write("NSP\1", 4);                    // magic + version
    StateWriter w(file);


    const std::string id = mapper->chunk_id();

    LOG(Info) << "------------------------------------" << std::endl;
    LOG(Info) << "[save_state] writing chunk "
              << id << '\n';

    w.begin(id);
    mapper->save_state(w);
    w.end();

    for (auto dev : serializables) {
        const std::string id = dev->chunk_id();

        LOG(Info) << "------------------------------------" << std::endl;
        LOG(Info) << "[save_state] writing chunk "
                  << id << '\n';

        w.begin(id);
        dev->save_state(w);
        w.end();
    }
}

bool Emulator::load_state(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Emulator::load_state: cannot open '" + filename + "' for reading");

    char hdr[4]; if (!file.read(hdr, 4) || std::memcmp(hdr, "NSP\1", 4)) return false;
    StateReader r(file);

    std::string id;
    uint32_t len;
    while (r.next(id,len)) {
        auto mapper_id = mapper->chunk_id();

        bool handled = false;

        if ( mapper_id == id) {
            LOG(Info) << "------------------------------------" << std::endl;
            LOG(Info) << "[load_state] loading chunk " << id << '\n';
            mapper->load_state(r);
            handled = true;
        }

        if (!handled) {
            for (auto *dev: serializables) {
                if (dev->chunk_id() == id) {
                    LOG(Info) << "------------------------------------" << std::endl;
                    LOG(Info) << "[load_state] loading chunk " << id << '\n';
                    dev->load_state(r);
                    handled = true;
                    break;
                }
            }
        }
        if (!handled) {
            LOG(Info) << "[load_state] skipping chunk " << id << '\n';
            r.skip_remainder();
        }
    }
    return true;
}

Emulator::Emulator(std::string rom_path) {
    // set the read callbacks
    bus.set_read_callback(PPUSTATUS, [&](void) { return ppu.get_status();          });
    bus.set_read_callback(PPUDATA,   [&](void) { return ppu.get_data(picture_bus); });
    bus.set_read_callback(JOY1,      [&](void) { return controllers[0].read();     });
    bus.set_read_callback(JOY2,      [&](void) { return controllers[1].read();     });
    bus.set_read_callback(OAMDATA,   [&](void) { return ppu.get_OAM_data();        });
    // set the write callbacks
    bus.set_write_callback(PPUCTRL,  [&](NES_Byte b) { ppu.control(b);                                             });
    bus.set_write_callback(PPUMASK,  [&](NES_Byte b) { ppu.set_mask(b);                                            });
    bus.set_write_callback(OAMADDR,  [&](NES_Byte b) { ppu.set_OAM_address(b);                                     });
    bus.set_write_callback(PPUADDR,  [&](NES_Byte b) { ppu.set_data_address(b);                                    });
    bus.set_write_callback(PPUSCROL, [&](NES_Byte b) { ppu.set_scroll(b);                                          });
    bus.set_write_callback(PPUDATA,  [&](NES_Byte b) { ppu.set_data(picture_bus, b);                               });
    bus.set_write_callback(OAMDMA,   [&](NES_Byte b) { cpu.skip_DMA_cycles(); ppu.do_DMA(bus.get_page_pointer(b)); });
    bus.set_write_callback(JOY1,     [&](NES_Byte b) { controllers[0].strobe(b); controllers[1].strobe(b);         });
    bus.set_write_callback(OAMDATA,  [&](NES_Byte b) { ppu.set_OAM_data(b);                                        });
    // set the interrupt callback for the PPU
    ppu.set_interrupt_callback([&]() { cpu.interrupt(bus, CPU::NMI_INTERRUPT); });
    // load the ROM from disk, expect that the Python code has validated it
    cartridge.loadFromFile(rom_path);
    // create the mapper based on the mapper ID in the iNES header of the ROM
    mapper = MapperFactory(&cartridge, [&](){ picture_bus.update_mirroring(); });
    // give the IO buses a pointer to the mapper
    bus.set_mapper(mapper);
    picture_bus.set_mapper(mapper);
}

void Emulator::step() {
    // render a single frame on the emulator
    for (int i = 0; i < CYCLES_PER_FRAME; i++) {
        // 3 PPU steps per CPU step
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        ppu.cycle(picture_bus);
        cpu.cycle(bus);
    }
}

}  // namespace NES
