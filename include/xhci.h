#ifndef __VIMOS_XHCI_H__
#define __VIMOS_XHCI_H__

#include "pci.h"

#define CAPLENGTH 0x00
#define HCSPARAMS1 0x04
#define HCSPARAMS2 0x08
#define HCCPARAMS1 0x10
#define DBOFF 0x14
#define RTSOFF 0x18

#define USBCMD 0x00
#define USBSTS 0x04
#define CRCR 0x18
#define DCBAAP 0x30
#define CONFIG 0x38
#define PORTSC(n) (0x400 + (0x10 * (n - 1)))

#define ERSTSZ(n) (0x20 + 0x08 + (n * 0x20))
#define ERSTBA(n) (0x20 + 0x10 + (n * 0x20))
#define ERDP(n) (0x20 + 0x18 + (n * 0x20))

struct event_ring_segment_table{
  uint64_t ring_segm_addr;
  uint16_t ring_segm_size;
  uint16_t resv0;
  uint32_t resv1;
}__attribute__((packed));

struct normal_trb{
  uint64_t buffer;
  uint32_t length : 17;
  uint32_t tdsize : 5;
  uint32_t target : 10;
  uint16_t c : 1;
  uint16_t ent : 1;
  uint16_t isp : 1;
  uint16_t flags : 2;
  uint16_t ioc : 1;
  uint16_t flags2 : 4;
  uint16_t type : 6;
  uint16_t resv;
}__attribute__((packed));

struct setup_stage_trb{
  uint8_t request_type;
  uint8_t request;
  uint16_t wvalue;
  uint16_t windex;
  uint16_t wlength;
  uint32_t trb_length : 17;
  uint32_t rsvd0 : 5;
  uint32_t target : 10;
  uint16_t c : 1;
  uint16_t flags : 5;
  uint16_t idt : 1;
  uint16_t rsvd2 : 3;
  uint16_t type : 6;
  uint16_t trt : 2;
  uint16_t rsvd1 : 14;
}__attribute__((packed));

struct data_stage_trb{
  uint64_t data_buf;
  uint32_t length : 17;
  uint32_t tdsize : 5;
  uint32_t target : 10;
  uint16_t c : 1;
  uint16_t flags : 4;
  uint16_t ioc : 1;
  uint16_t flags2 : 4;
  uint16_t type : 6;
  uint16_t dir : 1;
  uint16_t rsvd0 : 15;
}__attribute__((packed));

struct status_stage_trb{
  uint32_t rsvd0[2];
  uint32_t rsvd1 : 22;
  uint32_t target : 10;
  uint16_t c : 1;
  uint16_t flags : 4;
  uint16_t ioc : 1;
  uint16_t rsvd3 : 4;
  uint16_t type : 6;
  uint16_t dir : 1;
  uint16_t rsvd2 : 15;
}__attribute__((packed));

struct noop_command{
  uint64_t resv0;
  uint32_t resv1;
  uint16_t c : 1;
  uint16_t resv2 : 9;
  uint16_t type : 6;
  uint16_t slot_type : 5;
  uint16_t resv : 11;
}__attribute__((packed));

struct enable_slot_command{
  uint64_t resv0;
  uint32_t resv1;
  uint16_t c : 1;
  uint16_t resv2 : 9;
  uint16_t type : 6;
  uint16_t slot_type : 5;
  uint16_t resv : 11;
}__attribute__((packed));

struct address_device_command{
  uint64_t input_ctx_ptr;
  uint32_t rsvd0;
  uint16_t c : 1;
  uint16_t rsvd1 : 8;
  uint16_t bsr : 1;
  uint16_t type : 6;
  uint8_t rsvd2;
  uint8_t slot_id;
}__attribute__((packed));

struct configure_endpoint_command{
  uint64_t input_ctx_ptr;
  uint32_t rsvd0;
  uint16_t c : 1;
  uint16_t flags : 9;
  uint16_t type : 6;
  uint8_t rsvd2;
  uint8_t slot_id;
}__attribute__((packed));

struct evaluate_context_command{
  uint64_t input_ctx_ptr;
  uint32_t rsvd0;
  uint16_t c : 1;
  uint16_t rsvd1 : 8;
  uint16_t bsr : 1;
  uint16_t type : 6;
  uint8_t rsvd2;
  uint8_t slot_id;
}__attribute__((packed));

struct transfer_event{
  uint64_t ptr;
  uint32_t length : 24;
  uint32_t code : 8;
  uint16_t c : 1;
  uint16_t rsvd0 : 1;
  uint16_t ed : 1;
  uint16_t rsvd1 : 7;
  uint16_t type : 6;
  uint8_t endpoint : 5;
  uint8_t rsvd2 : 3;
  uint8_t slot_id;
}__attribute__((packed));

struct command_completion_event{
  uint64_t ptr;
  uint32_t param : 24;
  uint32_t code : 8;
  uint16_t c : 1;
  uint16_t rsvd0 : 9;
  uint16_t type : 6;
  uint8_t vf_id;
  uint8_t slot_id;
}__attribute__((packed));

struct port_status_change_event{
  uint32_t rsvd0 : 24;
  uint32_t port : 8;
  uint32_t rsvd1;
  uint32_t rsvd2 : 24;
  uint32_t code : 8;
  uint16_t c : 1;
  uint16_t rsvd3 : 9;
  uint16_t type : 6;
  uint16_t rsvd4;
}__attribute__((packed));

struct link_trb{
  uint64_t ring_ptr;
  uint32_t rsvd0 : 22;
  uint32_t target : 10;
  uint16_t c : 1;
  uint16_t tc : 1;
  uint16_t flags : 4;
  uint16_t rsvd1 : 4;
  uint16_t type : 6;
  uint16_t rsvd2;
}__attribute__((packed));

struct input_control_context{
  uint32_t d_flags;
  uint32_t a_flags;
  uint32_t resvd[5];
  uint8_t conf_value;
  uint8_t interface_num;
  uint8_t alternate_setting;
  uint8_t resv1;
}__attribute__((packed));

struct input_control_context64{
  struct input_control_context input;
  uint32_t rsvd0[4];
}__attribute__((packed));

struct slot_context{
  uint32_t route_string : 20;
  uint32_t speed : 4;
  uint32_t rsvd0 : 1;
  uint32_t mtt : 1;
  uint32_t hub : 1;
  uint32_t context_entries : 5;
  uint16_t max_exit_latency;
  uint8_t root_hub_port_num;
  uint8_t num_of_ports;
  uint8_t tt_hub_slot_id;
  uint8_t tt_port_num;
  uint16_t ttt : 2;
  uint16_t rsvd1 : 4;
  uint16_t interrupt_target : 10;
  uint32_t usb_dev_addr : 8;
  uint32_t rsvd2 : 19;
  uint32_t slot_state : 5;
  uint32_t rsvd3[4];
}__attribute__((packed));

struct slot_context64{
  struct slot_context slot;
  uint32_t rsvd0[4];
}__attribute__((packed));

struct endpoint_context{
  uint32_t ep_state : 3;
  uint32_t rsvd0 : 5;
  uint32_t mult : 2;
  uint32_t max_pstreams : 5;
  uint32_t lsa : 1;
  uint32_t interval : 8;
  uint32_t max_payload_hi : 8;
  uint16_t rsvd1 : 1;
  uint16_t cerr : 2;
  uint16_t ep_type : 3;
  uint16_t rsvd2 : 1;
  uint16_t hid : 1;
  uint16_t max_burst_size : 8;
  uint16_t max_packet_size;
  uint64_t tr_pointer;
  uint16_t average_length;
  uint16_t max_payload_lo;
  uint32_t rsvd4[3];
}__attribute__((packed));

struct endpoint_context64{
  struct endpoint_context device;
  uint32_t rsvd0[4];
}__attribute__((packed));

struct input_context{
  struct input_control_context icctx;
  struct slot_context slotctx;
  struct endpoint_context epctx[31];
}__attribute__((packed));

struct input_context64{
  struct input_control_context64 icctx;
  struct slot_context64 slotctx;
  struct endpoint_context64 epctx[31];
}__attribute__((packed));

void init_xhci(struct pci_device dev);
void usb_process_events(void);

void init_normal_trb(struct normal_trb *trb, uint64_t buf);
void init_setup_stage_trb(struct setup_stage_trb *trb);
void init_data_stage_trb(struct data_stage_trb *trb, uint64_t buf, uint32_t len);
void init_status_stage_trb(struct status_stage_trb *trb);
void init_address_device_command(struct address_device_command *trb, uint64_t ctx, uint8_t slot);

#define DESC_DEVICE 1
#define DESC_CONFIGURATION 2

#endif
