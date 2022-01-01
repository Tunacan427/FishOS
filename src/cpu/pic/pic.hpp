#pragma once

#include <types.hpp>

#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

#define ICW1_ICW4       0x01   // ICW4 (not) needed 
#define ICW1_SINGLE     0x02   // Single (cascade) mode
#define ICW1_INTERVAL4  0x04   // Call address interval 4 (8)
#define ICW1_LEVEL      0x08   // Level triggered (edge) mode
#define ICW1_INIT       0x10   // Initialization - required!
 
#define ICW4_8086       0x01   // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO       0x02   // Auto (normal) EOI
#define ICW4_BUF_SLAVE  0x08   // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C   // Buffered mode/master 
#define ICW4_SFNM       0x10   // Special fully nested (not)

#define PIC_EOI         0x20

namespace cpu {
namespace pic {

void remap(u8 offset1, u8 offset2);
void send_eoi(u8 irq);
void set_mask(u8 irq);
void clear_mask(u8 irq);

}
}
