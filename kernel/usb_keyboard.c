#include <stdbool.h>
#include <string.h>
#include "mem.h"
#include "paging.h"
#include "printk.h"
#include "usb.h"
#include "xhci.h"

bool usb_keyboard_found;

static unsigned char keytable0[0xff] = {
    0,    0,   0,   0,  'a', 'b', 'c', 'd',  'e',  'f',  'g',  'h', 'i', 'j', 'k', 'l',
  'm',  'n', 'o', 'p',  'q', 'r', 's', 't',  'u',  'v',  'w',  'x', 'y', 'z', '1', '2',
  '3',  '4', '5', '6',  '7', '8', '9', '0', '\n', 0x08, 0x08, '\t', ' ', '-', '=', '[',
  ']', '\\', '#', ';', '\'', '`', ',', '.',  '/',    0,    0,    0,   0,   0,   0,   0,
    0,    0,   0,   0,    0,   0,   0,   0,    0,    0,    0,    0,   0,   0,   0,   0,
    0,    0,   0,   0,  '/', '*', '-', '+', '\n',  '1',  '2',  '3', '4', '5', '6', '7',
  '8',  '9', '0', '.', '\\',   0,   0, '=',    0,    0,    0,    0,   0,   0,   0,   0,
    0,    0,   0,   0,    0,   0,   0,   0,    0,    0,    0,    0,   0,   0,   0,   0,
    0,    0,   0,   0,    0,   0,   0,   0,    0, '\\',    0,    0,   0,   0,   0,   0,
};

static unsigned char keytable1[0xff] = {
    0,   0,   0,   0,  'A', 'B', 'C', 'D',  'E',  'F',  'G',  'H', 'I', 'J', 'K', 'L',
  'M', 'N', 'O', 'P',  'Q', 'R', 'S', 'T',  'U',  'V',  'W',  'X', 'Y', 'Z', '!', '@',
  '#', '$', '%', '^',  '&', '*', '(', ')', '\n', 0x08, 0x08, '\t', ' ', '_', '+', '{',
  '}', '|', '~', ':',  '"', '~', '<', '>',  '?',    0,    0,    0,   0,   0,   0,   0,
    0,   0,   0,   0,    0,   0,   0,   0,    0,    0,    0,    0,   0,   0,   0,   0,
    0,   0,   0,   0,  '/', '*', '-', '+', '\n',  '1',  '2',  '3', '4', '5', '6', '7',
  '8', '9', '0', '.', '\\',   0,   0, '=',    0,    0,    0,    0,   0,   0,   0,   0,
    0,   0,   0,   0,    0,   0,   0,   0,    0,    0,    0,    0,   0,   0,   0,   0,
    0,   0,   0,   0,    0,   0,   0,   0,    0,  '|',    0,    0,   0,   0,   0,   0,
};

static struct ring *s_endpoint_ring;
static uint8_t s_slot_id;
static uint8_t s_dci;
static __attribute__((aligned(64))) uint8_t keybuf[64];

void init_usb_keyboard(struct ring *endpoint_ring, uint8_t slot_id, uint8_t dci){
  s_endpoint_ring = endpoint_ring;
  s_slot_id = slot_id;
  s_dci = dci;

  usb_keyboard_found = true;
//  printk("s_endpoint_ring->buf:%016X\n", s_endpoint_ring->buf);
  printk("s_slot_id:%d s_dci:%d\n", s_slot_id, s_dci);
  printk("usb_keyboard_found true\n");
}

static uint8_t last_pressed = 0x00;

char read_usb_keyboard(void){
//  printk("read_usb_keyboard\n");

  /* Test Code */
  struct normal_trb normal;
  memset(&normal, 0, sizeof(struct normal_trb));
  init_normal_trb(&normal, V2P(keybuf));
//  normal.type = 1;
//  normal.buffer = V2P(keybuf);
//  printk("buffer:%016X\n", normal.buffer);
  normal.length = 8;
//  normal.ioc = 1;
  normal.isp = 1;

  extern void write_ring(struct ring *ring, void *data);
  extern void write_xhci_doorbell(uint8_t reg, uint32_t val);
  write_ring(s_endpoint_ring, &normal);
  write_xhci_doorbell(s_slot_id, s_dci);

  while(1){
    extern bool er_isQueued(void);
    if(!er_isQueued()) break;

    extern volatile struct normal_trb *get_eventring(void);
    volatile struct normal_trb *trb = get_eventring();
    printk("read_usb_keyboard type:%d\n", trb->type);

    if(trb->type == 32){
      extern const char *get_trb_type(int i);
      extern const char *get_completion_msg(int i);

      struct transfer_event *event = (struct transfer_event *)trb;
      printk("Transfer Event code=%s(%d) slot=%d\n", get_completion_msg(event->code), event->code, event->slot_id);

      if(event->ptr){
        struct normal_trb *trb2 = (struct normal_trb *)P2V(event->ptr);
        printk("issuer:%s(%d)\n", get_trb_type(trb2->type), trb2->type);

        if(trb2->type == 1 && (event->code == 1 || event->code == 13)){
          unsigned char *key = keytable0;
          if(keybuf[0] & 0x22) key = keytable1;

#if 1
          printk("keybuf[0]:%02X ", keybuf[0]);

          for(int j = 2; j < 8; j++){
            if(keybuf[j] == 0) continue;

            printk("keybuf[%d]:%02X ", j, keybuf[j]);
          }
          printk("\n");
#endif

          return key[keybuf[2]];
        }
      }

      return 0;
    }else break;
  }

  return 0;
}
