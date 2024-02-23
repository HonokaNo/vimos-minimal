#include "xhci.h"
#include <stdbool.h>
#include <string.h>
#include "flags.h"
#include "mem.h"
#include "paging.h"
#include "pci.h"
#include "printk.h"
#include "usb.h"

static uint32_t xhci_caplength;
static uint32_t xhci_runtime_space;
static uint32_t xhci_doorbell;
static uint32_t xhci_extend_capability;

static __attribute__((aligned(64))) struct event_ring_segment_table erst;

enum{
  USB_DEVICE_STATUS_RESET,
  USB_DEVICE_STATUS_WAIT_ENABLE_SLOT,
  USB_DEVICE_STATUS_ENABLE_SLOT,
  USB_DEVICE_STATUS_ADDRESS_DEVICE,
  USB_DEVICE_STATUS_GET_DESCRIPTOR,
  USB_DEVICE_STATUS_END_OF_INITIALIZE
};
struct device_state{
  uint8_t state;
  struct device_state *next;
  struct ring *transfer_ring;
  volatile uint8_t buf[512];
};
static struct device_state device_status[255];
static struct device_state *start = NULL;
static struct device_state *state_last = NULL;

enum{
  USB_PORTCHECK,
  USB_FULLSPEED,
  USB_LOWSPEED,
  USB_HIGHSPEED,
  USB_SUPERSPEED
};
static uint8_t device_speed[255];

#if TEST_XHCI
  #define xhci_debug(...) printk(__VA_ARGS__)
#else
  #define xhci_debug(...)
#endif

static uint64_t mmio_base;

static struct ring cmd_ring;
static struct ring event_ring;

static bool csz = false;

static void new_ring(struct ring *ring){
  struct normal_trb *ring_buf = (struct normal_trb *)alloc_mem(1);
  if(!ring_buf) panic("alloc_mem failed!");
  memset(ring_buf, 0, 4096);
//  xhci_debug("transfer_ring:%016X\n", transfer_ring);

  ring->buf = ring_buf;
  ring->cycle_bit = true;
  ring->ring_size = 4096 / sizeof(struct normal_trb);
  ring->ring_index = 0;
}

//static struct device_context *device_contexts[];
static volatile void **device_contexts;

static uint32_t read_xhci(uint16_t reg){
  return *(volatile uint32_t *)(mmio_base + reg);
}

static uint32_t read_xhci_operation(uint16_t reg){
  return *(volatile uint32_t *)(mmio_base + xhci_caplength + reg);
}

static void write_xhci_operation(uint16_t reg, uint32_t val){
  *(volatile uint32_t *)(mmio_base + xhci_caplength + reg) = val;
}

static uint32_t read_xhci_runtime(uint16_t reg){
  return *(volatile uint32_t *)(mmio_base + xhci_runtime_space + reg);
}

static void write_xhci_runtime(uint16_t reg, uint32_t val){
  *(volatile uint32_t *)(mmio_base + xhci_runtime_space + reg) = val;
}

//static void write_xhci_doorbell(uint8_t reg, uint32_t val){
void write_xhci_doorbell(uint8_t reg, uint32_t val){
  ((volatile uint32_t *)(mmio_base + xhci_doorbell))[reg] = val;
}

static void write_ring_static(void *ring, int index, void *data, bool cycle_bit){
  uint32_t *trb = (uint32_t *)ring + (index * sizeof(uint32_t));
  uint32_t *tdata = (uint32_t *)data;

  for(int i = 0; i < 3; i++){
    trb[i] = tdata[i];
  }

  trb[3] = tdata[3] | (uint32_t)cycle_bit;
}

//static void write_ring(struct ring *ring, void *data){
void write_ring(struct ring *ring, void *data){
  write_ring_static(ring->buf, ring->ring_index, data, ring->cycle_bit);

  ring->ring_index++;

  if(ring->ring_index == ring->ring_size - 1){
    struct link_trb ltrb;
    memset(&ltrb, 0, sizeof(struct link_trb));
    ltrb.ring_ptr = V2P(ring->buf);
    ltrb.type = 6;
    ltrb.tc = 1;

    write_ring_static(ring->buf, ring->ring_index, &ltrb, ring->cycle_bit);

    ring->ring_index = 0;
    ring->cycle_bit = !ring->cycle_bit;
  }
}

static volatile struct normal_trb *getTRB(void){
  uint64_t erdp = read_xhci_runtime(ERDP(0) + 0) | ((uint64_t)read_xhci_runtime(ERDP(0) + 4) << 32);

  volatile struct normal_trb *trb = (volatile struct normal_trb *)P2V(erdp & ~0x0f);
  return trb;
}

static void write_event_ring(void){
  volatile struct normal_trb *trb = getTRB();
  trb++;

  if((uint64_t)trb == (uint64_t)event_ring.buf + 4096){
    event_ring.cycle_bit = !event_ring.cycle_bit;

    write_xhci_runtime(ERDP(0) + 0, V2P(event_ring.buf) >> 0 | 0x08);
    write_xhci_runtime(ERDP(0) + 4, V2P(event_ring.buf) >> 32);
  }else{
    write_xhci_runtime(ERDP(0) + 0, V2P(trb) >> 0 | 0x08);
    write_xhci_runtime(ERDP(0) + 4, V2P(trb) >> 32);
  }
}

//static bool er_isQueued(void){
bool er_isQueued(void){
  return getTRB()->c == event_ring.cycle_bit;
}

volatile struct normal_trb *get_eventring(void){
  volatile struct normal_trb *trb = getTRB();

  while(!er_isQueued());

  write_event_ring();

  return trb;
}

static void get_descriptor(struct ring *transfer_ring, uint8_t slot_id, uint16_t desc_type, uint8_t desc_index, volatile uint8_t *buf, size_t buf_len){
  struct setup_stage_trb setup_trb;
  memset(&setup_trb, 0, sizeof(struct setup_stage_trb));
  init_setup_stage_trb(&setup_trb);
  setup_trb.request_type = 0x80;
  setup_trb.request = 6;  /* GET_DESCRIPTOR */
  setup_trb.wvalue = (desc_type << 8) | desc_index;  /* DESC_DEVICE | desc_index */
  setup_trb.windex = 0;
  setup_trb.wlength = buf_len;
  setup_trb.trt = 3;  /* IN Data Storage */

  struct data_stage_trb data_trb;
  memset(&data_trb, 0, sizeof(struct data_stage_trb));
  init_data_stage_trb(&data_trb, V2P(buf), buf_len);
  data_trb.tdsize = 0;
//  data_trb.ioc = 0;
  data_trb.dir = 1;

  struct status_stage_trb status_trb;
  memset(&status_trb, 0, sizeof(struct status_stage_trb));
  init_status_stage_trb(&status_trb);
  status_trb.dir = 0;
//  status_trb.ioc = 1;

  write_ring(transfer_ring, &setup_trb);
  write_ring(transfer_ring, &data_trb);
  write_ring(transfer_ring, &status_trb);
  write_xhci_doorbell(slot_id, 1);
}

static void get_descriptor_noreport(struct ring *transfer_ring, uint8_t slot_id, uint16_t desc_type, uint8_t desc_index, volatile uint8_t *buf, size_t buf_len){
  struct setup_stage_trb setup_trb;
  memset(&setup_trb, 0, sizeof(struct setup_stage_trb));
  init_setup_stage_trb(&setup_trb);
  setup_trb.request_type = 0x80;
  setup_trb.request = 6;  /* GET_DESCRIPTOR */
  setup_trb.wvalue = (desc_type << 8) | desc_index;  /* DESC_DEVICE | desc_index */
  setup_trb.windex = 0;
  setup_trb.wlength = buf_len;
  setup_trb.trt = 3;  /* IN Data Storage */

  struct data_stage_trb data_trb;
  memset(&data_trb, 0, sizeof(struct data_stage_trb));
  init_data_stage_trb(&data_trb, V2P(buf), buf_len);
  data_trb.tdsize = 0;
  data_trb.dir = 1;

  struct status_stage_trb status_trb;
  memset(&status_trb, 0, sizeof(struct status_stage_trb));
  init_status_stage_trb(&status_trb);
  status_trb.dir = 0;

  write_ring(transfer_ring, &setup_trb);
  write_ring(transfer_ring, &data_trb);
  write_ring(transfer_ring, &status_trb);
  write_xhci_doorbell(slot_id, 1);
}

void init_xhci(struct pci_device dev){
  xhci_debug("xHCI is found.\n");

  uint64_t addr = ((uint64_t)dev.bar1 << 32) | (dev.bar0 & ~0x0f);
  mmio_base = P2V(addr);

//  xhci_debug("addr:%016X\n", addr);
  xhci_debug("mmio_base:%016X\n", mmio_base);

  uint32_t caplength = read_xhci(CAPLENGTH);
//  xhci_debug("CAPLENGTH:%02X\n", caplength & 0xff);
  xhci_caplength = caplength & 0xff;

  uint32_t rtsoff = read_xhci(RTSOFF);
//  xhci_debug("RTSOFF:%08X\n", rtsoff);
  xhci_runtime_space = rtsoff;

  uint32_t dboff = read_xhci(DBOFF);
//  xhci_debug("DBOFF:%08X\n", dboff);
  xhci_doorbell = dboff;

  uint32_t hccparams = read_xhci(HCCPARAMS1);
//  xhci_debug("HCCPARAMS:%08X\n", hccparams);
//  xhci_debug("extend_capabilities_offset:%08X\n", (((hccparams >> 16) & 0xffff) << 2));
  xhci_extend_capability = (((hccparams >> 16) & 0xffff) << 2);

//  xhci_debug("extend_capabilities_offset:%016X\n", mmio_base + xhci_extend_capability);
  uint16_t off = 0;

  while(1){
    uint32_t v = *(volatile uint32_t *)(mmio_base + xhci_extend_capability + off);
//    xhci_debug("v:%08X\n", v);

    /* USB Legacy Support */
    if((v & 0xff) == 1){
      v |= (1 << 24);   /* HC OS Owned Semaphore */
      *(volatile uint32_t *)(mmio_base + xhci_extend_capability + off) = v;
    }

    /* support protocol */
    if((v & 0xff) == 2){
//      xhci_debug("major:%d minor:%d\n", (v >> 24) & 0xff, (v >> 16) & 0xff);
//      xhci_debug("0x08:%08X\n", *(volatile uint32_t *)(mmio_base + xhci_extend_capability + off + 0x08));
      uint32_t offset08 = *(volatile uint32_t *)(mmio_base + xhci_extend_capability + off + 0x08);
      uint32_t port_offset = offset08 & 0xff;
      uint32_t port_count = (offset08 >> 8) & 0xff;
      uint32_t psic = (offset08 >> 28) & 0x0f;
      xhci_debug("port_offset:%d port_count:%d psic:%d\n", port_offset, port_count, psic);

#if 0
      for(uint32_t i = 0; i < psic; i++){
        uint32_t psi = *(volatile uint32_t *)(mmio_base + xhci_extend_capability + off + 0x10 + (i * 4));
        xhci_debug("psi:%08X\n", psi);
        xhci_debug("PSIV:%02X PSIE:%d\n", psi & 0x0f, (psi >> 4) & 0x03);
        xhci_debug("Speed is %d%s\n", psi & 0xff, ((psi >> 4) & 0x03) == 0 ? "bps" : ((psi >> 4) & 0x03) == 1 ? "Kb/s" : ((psi >> 4) & 0x03) == 2 ? "Mb/s" : "Gb/s");
      }
#endif

#if 0
      for(uint32_t i = port_offset; i < port_offset + port_count; i++){
        if(((v >> 24) & 0xff) == 3){
          device_speed[i - 1] = USB_SUPERSPEED;
        }else{
          device_speed[i - 1] = USB_HIGHSPEED;
        }
      }
#endif
    }

    off = ((v >> 8) & 0xffff) << 2;
    if(!off) break;
  }

  csz = (hccparams & 0x04);
//  xhci_debug("sizeof(SlotContext):%d csz:%d\n", sizeof(struct slot_context), (hccparams & 0x04) ? 64 : 32);
//  xhci_debug("sizeof(EndpointContext):%d csz:%d\n", sizeof(struct endpoint_context), (hccparams & 0x04) ? 64 : 32);

  /* PAGESIZE */
  uint32_t pagesize = read_xhci_operation(0x08);
//  xhci_debug("PAGESIZE:%08X\n", pagesize);

  uint32_t usbsts = read_xhci_operation(USBSTS);
//  xhci_debug("hch:%01X\n", usbsts & 0x01);

  if(!(usbsts & 0x01)) panic("USBSTS.HCH is 0!");

  uint32_t usbcmd = read_xhci_operation(USBCMD);
  usbcmd |= 0x02;  /* HCRST */
  write_xhci_operation(USBCMD, usbcmd);

  usbcmd = read_xhci_operation(USBCMD);
  /* wait change USBCMD.HCRST is 0 */
  while(usbcmd & 0x02){
    usbcmd = read_xhci_operation(USBCMD);
  }

  usbsts = read_xhci_operation(USBSTS);
  /* wait change USBSTS.CNR is 0 */
  while(usbsts & 0x800){
    usbsts = read_xhci_operation(USBSTS);
  }

  uint32_t hcsparams1 = read_xhci(HCSPARAMS1);
//  xhci_debug("MaxSlots:%d\n", hcsparams1 & 0xff);

  uint32_t config = read_xhci_operation(CONFIG);
  config &= ~0xff;
  config |= (hcsparams1 & 0xff);
  write_xhci_operation(CONFIG, config);

//  xhci_debug("need size:%d\n", ((hcsparams1 & 0xff) + 1) * sizeof(void *));

  /* 256(Slots) * sizeof(device_context *) = 2048 */
  device_contexts = alloc_mem(1);
  if(!device_contexts) panic("alloc_mem failed!");
//  xhci_debug("device_contexts:%016X\n", (uint64_t)device_contexts);
  memset(device_contexts, 0, 4096);

#if 1
  /* set scratch pad */
  uint32_t params2 = read_xhci(HCSPARAMS2);
  uint32_t scratchpads = ((params2 >> 27) & 0x1f) | (((params2 >> 21) & 0x1f) << 5);
//  xhci_debug("scratchpads:%d\n", scratchpads);
//  xhci_debug("need_page:%d\n", (scratchpads * 8 + 4095) / 4096);
  void **scratchpad_array = (void **)alloc_mem((scratchpads * 8 + 4095) / 4096);
  if(!scratchpad_array) panic("alloc_mem failed!");
  for(uint32_t i = 0; i < scratchpads; i++){
    /* TODO:support any PAGESIZE */
    if(pagesize == 0x01){
      void *scratchpad_array_buf = alloc_mem(1);
      if(!scratchpad_array_buf) panic("alloc_mem failed!");

//      xhci_debug("scratchpad_array_buf:%016X\n", scratchpad_array_buf);
      scratchpad_array[i] = (void *)V2P(scratchpad_array_buf);
    }else panic("scratchpad panic!\n");
  }
//  device_contexts[0] = (struct device_context *)scratchpad_array;
  device_contexts[0] = (struct device_context *)V2P(scratchpad_array);
//  xhci_debug("device_contexts[0]:%016X\n", device_contexts[0]);
#endif

  write_xhci_operation(DCBAAP + 0, (V2P(device_contexts) >>  0) & 0xffffffff);
  write_xhci_operation(DCBAAP + 4, (V2P(device_contexts) >> 32) & 0xffffffff);
//  xhci_debug("DCBAAP:%08X %08X\n", read_xhci_operation(DCBAAP + 4), read_xhci_operation(DCBAAP + 0));

  new_ring(&cmd_ring);

  write_xhci_operation(CRCR + 0, ((V2P(cmd_ring.buf) >>  0) & 0xffffffff) | cmd_ring.cycle_bit);
  write_xhci_operation(CRCR + 4, (V2P(cmd_ring.buf) >> 32) & 0xffffffff);
//  xhci_debug("CRCR:%08X %08X\n", read_xhci_operation(CRCR + 4), read_xhci_operation(CRCR + 0));

  new_ring(&event_ring);

  erst.ring_segm_addr = V2P(event_ring.buf);
  erst.ring_segm_size = event_ring.ring_size;

  write_xhci_runtime(ERSTSZ(0), 1);
  write_xhci_runtime(ERDP(0) + 0, (erst.ring_segm_addr) >>  0);
  write_xhci_runtime(ERDP(0) + 4, (erst.ring_segm_addr) >> 32);
  write_xhci_runtime(ERSTBA(0) + 0, (V2P(&erst) >>  0));
  write_xhci_runtime(ERSTBA(0) + 4, (V2P(&erst) >> 32));

//  xhci_debug("ERDP:%08X %08X\n", read_xhci_runtime(ERDP(0) + 4), read_xhci_runtime(ERDP(0) + 0));

  usbcmd = read_xhci_operation(USBCMD);
  usbcmd |= 0x01;  /* R/S */
  write_xhci_operation(USBCMD, usbcmd);

  usbsts = read_xhci_operation(USBSTS);
  /* wait change USBSTS.HCH is 0 */
  while(usbsts & 0x01){
    usbsts = read_xhci_operation(USBSTS);
  }

  xhci_debug("xHCI is Run\n");

  extern void scroll_screen(void);
  scroll_screen();

#if 1
//  uint8_t num_ports = (read_xhci(HCSPARAMS1) >> 24) & 0xff;
//  xhci_debug("num_ports:%d\n", num_ports);

//  for(int i = 1; i < num_ports + 1; i++){
    /* all port reset */
//    uint32_t portsc = read_xhci_operation(PORTSC(i));

#if 0
    portsc &= 0x0e00c3e0;
    portsc |= (1 << 17);  /* Connect Status Change(Clear) */
    portsc |= 0x10;  /* Port Reset */

    write_xhci_operation(PORTSC(i), portsc);

    /* wait PORTSC.PED to 1 */
//    while(!(read_xhci_operation(PORTSC(i) & 0x02)));
    /* wait PORTSC.PRC to 1 */
    while(!(read_xhci_operation(PORTSC(i) & (1 << 21))));

    xhci_debug("resetting port %d... %08X\n", i, read_xhci_operation(PORTSC(i)));

    /* after generate Port Status Change Event, process in usb_process_events */
    /* Maximum number of ports is 255, so the event ring never overflows */
#endif
//  }
#endif

//  xhci_debug("usb_process_events\n");
//  usb_process_events();
}

const char *get_trb_type(int i){
  switch(i){
    case 1: return "Normal";
    case 3: return "Data Stage";
    case 4: return "Status Stage";
    case 9: return "Enable Slot";
    case 11: return "Address Device";
    case 12: return "Configure Endpoint";
    case 13: return "Evaluate Context";
    case 32: return "Transfer";
    case 33: return "Command Completion";
    case 34: return "Port Status Change";
  }

  return "?";
}

const char *get_completion_msg(int i){
  switch(i){
    case 0: return "Invalid";
    case 1: return "Success";
    case 3: return "Bubble Detected";
    case 4: return "USB Transaction";
    case 5: return "TRB";
    case 6: return "Stall";
    case 11: return "Slot Not Enabled";
    case 12: return "Endpoint Not Enabled";
    case 13: return "Short Packet";
    case 17: return "Parameter";
    case 19: return "Context State";
    case 20: return "No Ping Response";
    case 21: return "Event Ring Full";
  }

  return "?";
}

static int16_t current_port = -1;

static void handle_enable_slot(struct command_completion_event *event, uint8_t port_id){
  xhci_debug("Enable Slot slot:%d\n", event->slot_id);

//  struct input_context *input_ctx = (struct input_context *)alloc_mem(1);
  void *input_ctx_buf = alloc_mem(2);
//  if(!input_ctx) panic("alloc_mem failed!");
  if(!input_ctx_buf) panic("alloc_mem failed!");
//  memset(input_ctx, 0, sizeof(struct input_context));
  memset(input_ctx_buf, 0, 4096 * 2);
  struct input_context *input_ctx = (struct input_context *)(input_ctx_buf + 4096 - 0x20);
//  xhci_debug("input_ctx:%016X\n", input_ctx);

  /* enable to Slot Context, EP Context 0 */
  input_ctx->icctx.a_flags = 0x03;

//  device_contexts[event->slot_id] = (struct device_context *)&input_ctx->slotctx;
//  xhci_debug("event->slot_id:%d\n", event->slot_id);
  device_contexts[event->slot_id] = (void *)V2P(&input_ctx->slotctx);
//  xhci_debug("input_context:%016X\n", input_ctx);
  xhci_debug("device_contexts[%d]:%016X\n", event->slot_id, V2P(&input_ctx->slotctx));
//  xhci_debug("device_contexts:%016X\n", V2P(&input_ctx->slotctx) & ~0x3f);

//  xhci_debug("start:%016X device_status[0]:%016X\n", start, &device_status[0]);
  input_ctx->slotctx.route_string = 0;
#if 0
  input_ctx->slotctx.root_hub_port_num = port_id;
#else
  input_ctx->slotctx.root_hub_port_num = current_port;
#endif
  input_ctx->slotctx.context_entries = 1;
#if 1
//  xhci_debug("PORTSC(%d):%08X\n", port_id, read_xhci_operation(PORTSC(port_id)));
  xhci_debug("PSIV:%02X\n", (read_xhci_operation(PORTSC(port_id)) >> 10) & 0x0f);
  input_ctx->slotctx.speed = (read_xhci_operation(PORTSC(port_id)) >> 10) & 0x0f;
#endif

#if 0
  device_status[port_id - 1].transfer_ring = (struct ring *)alloc_mem(1);
  if(!device_status[port_id - 1].transfer_ring) panic("alloc_mem failed!");
  new_ring(device_status[port_id - 1].transfer_ring);
//  xhci_debug("device_status[%d]:%016X\n", port_id - 1, device_status[port_id - 1].transfer_ring);
#else
  device_status[current_port - 1].transfer_ring = (struct ring *)alloc_mem(1);
  if(!device_status[current_port - 1].transfer_ring) panic("alloc_mem failed!");
  new_ring(device_status[current_port - 1].transfer_ring);
//  xhci_debug("device_status[%d]:%016X\n", current_port - 1, device_status[current_port - 1].transfer_ring);
#endif

  input_ctx->epctx[0].ep_type = 4;
#if 0
  input_ctx->epctx[0].tr_pointer = V2P(device_status[port_id - 1].transfer_ring->buf) | (uint32_t)device_status[port_id - 1].transfer_ring->cycle_bit;
#else
  input_ctx->epctx[0].tr_pointer = V2P(device_status[current_port - 1].transfer_ring->buf) | (uint32_t)device_status[current_port - 1].transfer_ring->cycle_bit;
#endif
  input_ctx->epctx[0].max_burst_size = 0;
  input_ctx->epctx[0].mult = 0;
  input_ctx->epctx[0].interval = 0;
  input_ctx->epctx[0].max_pstreams = 0;
  input_ctx->epctx[0].cerr = 3;
  /* ref. xHCI specification 1.2, section 4.14.1.1 */
  input_ctx->epctx[0].average_length = 8;

  if(input_ctx->slotctx.speed == 4) input_ctx->epctx[0].max_packet_size = 512;
  else if(input_ctx->slotctx.speed == 3) input_ctx->epctx[0].max_packet_size = 64;
  else input_ctx->epctx[0].max_packet_size = 8;

  struct address_device_command atrb;
#if 1
  memset(&atrb, 0, sizeof(struct address_device_command));
  init_address_device_command(&atrb, V2P(input_ctx), event->slot_id);
  /* Set Default Device State, Enable Default Control Endpoint */
//  if(input_ctx->slotctx.speed == 1) atrb.bsr = 1;

  write_ring(&cmd_ring, &atrb);
  write_xhci_doorbell(0, 0);

//  xhci_debug("slot_state:%d\n", input_ctx->slotctx.slot_state);

//  xhci_debug("dcbaap:%016X\n", device_contexts[event->slot_id]);
  struct endpoint_context *ep_ctx = (struct endpoint_context *)(P2V(device_contexts[event->slot_id]) + 0x20);
  xhci_debug("ep_ctx->max_packet_size:%d\n", ep_ctx->max_packet_size);

  /* Full-Speed */
  if(input_ctx->slotctx.speed == 1){
    get_descriptor_noreport(device_status[port_id - 1].transfer_ring, event->slot_id, DESC_DEVICE, 0, device_status[port_id - 1].buf, 8);

    volatile struct device_descriptor *desc = (volatile struct device_descriptor *)device_status[port_id - 1].buf;
    xhci_debug("length:%d\n", desc->bLength);
    xhci_debug("bMaxPacketSize0:%d\n", desc->bMaxPacketSize0);
//    if(desc->bLength >= 1){
    if(desc->bMaxPacketSize0 != 0){
      input_ctx->epctx[0].max_packet_size = desc->bMaxPacketSize0;

      struct evaluate_context_command evaluate_context_cmd;
      memset(&evaluate_context_cmd, 0, sizeof(struct evaluate_context_command));

      evaluate_context_cmd.input_ctx_ptr = V2P(input_ctx);
      evaluate_context_cmd.type = 13;
      evaluate_context_cmd.slot_id = event->slot_id;

      write_ring(&cmd_ring, &evaluate_context_cmd);
      write_xhci_doorbell(0, 0);
    }
#endif
  }

//  device_speed[port_id - 1] = input_ctx->slotctx.speed;
  device_speed[current_port - 1] = input_ctx->slotctx.speed;

//    memset(&atrb, 0, sizeof(struct address_device_command));
//    init_address_device_command(&atrb, V2P(input_ctx), event->slot_id);

//    write_ring(&cmd_ring, &atrb);
//    write_xhci_doorbell(0, 0);

//  xhci_debug("slot_state:%d\n", input_ctx->slotctx.slot_state);

#if 0
  device_status[port_id - 1].state = USB_DEVICE_STATUS_ADDRESS_DEVICE;
#else
  device_status[current_port - 1].state = USB_DEVICE_STATUS_ADDRESS_DEVICE;
#endif
}

int get_highest_bit(uint8_t b){
  int r = 0;
  while(b){
    r++;
    b >>= 1;
  }

  return r;
}

void usb_process_events(void){
  while(1){
    if(!er_isQueued()) return;

    volatile struct normal_trb *trb = get_eventring();
//    xhci_debug("trb->type:%d\n", trb->type);

    /* Transfer Event */
    if(trb->type == 32){
      struct transfer_event *event = (struct transfer_event *)trb;
      xhci_debug("Transfer Event code=%s(%d)\n", get_completion_msg(event->code), event->code);
      xhci_debug("Issuer TRB:%s(%d)\n", get_trb_type(((struct normal_trb *)P2V(event->ptr))->type), ((struct normal_trb *)P2V(event->ptr))->type);

//      uint8_t port_id = (uint8_t)(((uint64_t)start - (uint64_t)&device_status[0]) / sizeof(struct device_state)) + 1;
//      xhci_debug("port_id:%d\n", port_id);
      uint8_t port_id = ((struct slot_context *)P2V(device_contexts[event->slot_id]))->root_hub_port_num;
      xhci_debug("slot_id:%d -> port_id:%d\n", event->slot_id, port_id);

      /* Success / Short Packet */
      if(event->code == 1 || event->code == 13){
        if(device_status[port_id - 1].state == USB_DEVICE_STATUS_GET_DESCRIPTOR){
          struct device_descriptor *dev_desc = (struct device_descriptor *)device_status[port_id - 1].buf;
          xhci_debug("dev_desc:%016X\n", dev_desc);

          volatile uint8_t *dev_buf = device_status[port_id - 1].buf;
          int dev_buf_len = sizeof(device_status[port_id - 1].buf);

          uint8_t classCode = dev_desc->bDeviceClass;
          uint8_t subClassCode = dev_desc->bDeviceSubClass;
          uint8_t interface = 0;

//          xhci_debug("\nbDeviceClass:%02X bDeviceSubClass:%02X\n", dev_desc->bDeviceClass, dev_desc->bDeviceSubClass);
//          xhci_debug("idVendor:%04X idProduct:%04X\n", dev_desc->idVendor, dev_desc->idProduct);
          uint8_t num_configs = dev_desc->bNumConfigurations;
          xhci_debug("num_configs:%d\n", num_configs);

          for(uint8_t desc_index = 0; desc_index < num_configs; desc_index++){
            get_descriptor_noreport(device_status[port_id - 1].transfer_ring, event->slot_id, DESC_CONFIGURATION, desc_index, dev_buf, dev_buf_len);

            struct configuration_descriptor *conf_desc = (struct configuration_descriptor *)dev_buf;

            uint16_t total_length = conf_desc->wTotalLength;
            uint8_t configuration_value = conf_desc->bConfigurationValue;
            xhci_debug("value:%02X length:%d\n", configuration_value, total_length);

            /* set_configuration */
            struct setup_stage_trb setup_trb;
            memset(&setup_trb, 0, sizeof(struct setup_stage_trb));
            init_setup_stage_trb(&setup_trb);
            setup_trb.request_type = 0;
            setup_trb.request = 9;  /* SET_CONFIGURATION */
            setup_trb.wvalue = configuration_value;
            setup_trb.windex = 0;
            setup_trb.wlength = 0;
            setup_trb.trt = 0;  /* No Data Stage */

            struct status_stage_trb status_trb;
            memset(&status_trb, 0, sizeof(struct status_stage_trb));
            init_status_stage_trb(&status_trb);
            status_trb.dir = 1;
            status_trb.ioc = 1;

//            write_ring(device_status[port_id - 1].transfer_ring, &setup_trb);
//            write_ring(device_status[port_id - 1].transfer_ring, &status_trb);
//            write_xhci_doorbell(event->slot_id, 1);  /* DCI of the Default Control Pipe */

            struct input_context *input_ctx = (struct input_context *)alloc_mem(1);
            if(!input_ctx) panic("alloc_mem failed!");
            memset(input_ctx, 0, sizeof(struct input_context));

//            xhci_debug("device_contexts:%016X\n", P2V(device_contexts[event->slot_id]));
            /* copy slot context */
//            memcpy(&input_ctx->slotctx, device_contexts[event->slot_id], sizeof(struct endpoint_context));
            memcpy(&input_ctx->slotctx, (void *)P2V(device_contexts[event->slot_id]), sizeof(struct slot_context));

            xhci_debug("root_hub_port_num test:%d\n", input_ctx->slotctx.root_hub_port_num);

            input_ctx->icctx.a_flags = 0x01;

            bool is_set_configuration = false;
            uint8_t ep_in_dci;

            /* parse device descriptor */
            volatile uint8_t *p = dev_buf + dev_desc->bLength;
            while(p < dev_buf + total_length){
              uint8_t desc_type = p[1];
//              xhci_debug("desc_type:%d\n", desc_type);
//              xhci_debug("desc_length:%d\n", p[0]);

              if(!p[0]) break;

              /* DESC_INTERFACE */
              if(desc_type == 4){
                struct interface_descriptor *interface_desc = (struct interface_descriptor *)p;
//                uint8_t interface_class_code = interface_desc->bInterfaceClass;
//                uint8_t interface_subclass_code = interface_desc->bInterfaceSubClass;
//                uint8_t interface_protocol = interface_desc->bInterfaceProtocol;
                classCode = interface_desc->bInterfaceClass;
                subClassCode = interface_desc->bInterfaceSubClass;
                interface = interface_desc->bInterfaceProtocol;

                xhci_debug("class:%02X subclass:%02X interface:%02X\n", classCode, subClassCode, interface);

                /* HID && Boot Interface && Keyboard */
//                if(boot_keyboard_config_value == 0 && interface_class_code == 3 && interface_subclass_code == 1 && interface_protocol == 1){
//                  boot_keyboard_config_value = configuration_value;
//                }
              /* DESC_ENDPOINT */
              }else if(desc_type == 5){
                struct endpoint_descriptor *endpoint_desc = (struct endpoint_descriptor *)p;
                uint8_t ep_addr = endpoint_desc->bEndpointAddress;
                uint8_t direction = ep_addr >> 7;
                uint8_t attr = endpoint_desc->bmAttributes;
                bool interrupt = (attr & 0x03) == 3;
                bool isochronous = (attr & 0x03) == 1;
                bool bulk = (attr & 0x03) == 2;

                xhci_debug("ep_addr:%02X direction:%d attr:%02X interrupt:%d\n", ep_addr, direction, attr, interrupt);

                /* IN && Interrupt */
//                if(boot_keyboard_config_value != 0 && direction == 1 && interrupt == 1){
//                  memcpy(ep_key_in, p, 7);
//                }

                uint8_t ep_in_num = endpoint_desc->bEndpointAddress & 0x0f;
                ep_in_dci = 2 * ep_in_num + direction;
                xhci_debug("ep_in_num:%d ep_in_dci:%d\n", ep_in_num, ep_in_dci);

                input_ctx->icctx.a_flags |= (1 << ep_in_dci);
                input_ctx->slotctx.context_entries = input_ctx->slotctx.context_entries <= ep_in_dci ? ep_in_dci : input_ctx->slotctx.context_entries;

                struct ring *endpoint = alloc_mem(1);
                if(!endpoint) panic("alloc_mem failed!");
                new_ring(endpoint);

//                xhci_debug("wMaxPacketSize:%d\n", endpoint_desc->wMaxPacketSize);
//                xhci_debug("bInterval:%d\n", endpoint_desc->bInterval);

                if(interrupt) input_ctx->epctx[ep_in_dci - 1].ep_type = direction ? 7 : 3;  /* Interrupt In / Interrupt Out */
                if(isochronous) input_ctx->epctx[ep_in_dci - 1].ep_type = direction ? 5 : 1;  /* Isoch In / Isoch Out */
                if(bulk) input_ctx->epctx[ep_in_dci - 1].ep_type = direction ? 6 : 2;  /* Bulk In / Bulk Out */
                input_ctx->epctx[ep_in_dci - 1].tr_pointer = V2P(endpoint->buf) | (uint32_t)endpoint->cycle_bit;
                input_ctx->epctx[ep_in_dci - 1].max_packet_size = endpoint_desc->wMaxPacketSize;
//                input_ctx->epctx[ep_in_dci - 1].max_burst_size = 0;
                input_ctx->epctx[ep_in_dci - 1].max_burst_size = (endpoint_desc->wMaxPacketSize & 0x1800) >> 11;
                input_ctx->epctx[ep_in_dci - 1].mult = 0;

                uint32_t emit_payload = endpoint_desc->wMaxPacketSize * (input_ctx->epctx[ep_in_dci - 1].max_burst_size + 1);
                input_ctx->epctx[ep_in_dci - 1].max_payload_lo = emit_payload & 0xffff;
                input_ctx->epctx[ep_in_dci - 1].max_payload_hi = (emit_payload >> 16) & 0xff;

                if(device_speed[port_id - 1] == USB_FULLSPEED || device_speed[port_id - 1] == USB_LOWSPEED){
                  if(isochronous) input_ctx->epctx[ep_in_dci - 1].interval = endpoint_desc->bInterval + 2;
                  else input_ctx->epctx[ep_in_dci - 1].interval = get_highest_bit(endpoint_desc->bInterval) + 3;
                }else{
                  input_ctx->epctx[ep_in_dci - 1].interval = endpoint_desc->bInterval - 1;
                }

                input_ctx->epctx[ep_in_dci - 1].max_pstreams = 0;
                if(isochronous) input_ctx->epctx[ep_in_dci - 1].cerr = 0;
                else input_ctx->epctx[ep_in_dci - 1].cerr = 3;
//                input_ctx->epctx[ep_in_dci - 1].average_length = 1;
                /* ref. xHCI specification 1.2, section 4.14.1.1 */
                input_ctx->epctx[ep_in_dci - 1].average_length = 3 * 1024;
                if(interrupt) input_ctx->epctx[ep_in_dci - 1].average_length = 1024;

                if(classCode == 3 && subClassCode == 1 && interface == 1 && direction == 1 && interrupt == 1){
                  /* set this conguration */
                  write_ring(device_status[port_id - 1].transfer_ring, &setup_trb);
                  write_ring(device_status[port_id - 1].transfer_ring, &status_trb);
                  write_xhci_doorbell(event->slot_id, 1);  /* DCI of the Default Control Pipe */
                  is_set_configuration = true;

                  init_usb_keyboard(endpoint, event->slot_id, ep_in_dci);
                }
              }

              p += p[0];
            }

            if(is_set_configuration){
//              xhci_debug("aflags:%08X\n", input_ctx->icctx.a_flags);
              xhci_debug("context_entries:%d\n", input_ctx->slotctx.context_entries);

              struct configure_endpoint_command configure_endpoint_cmd;
              memset(&configure_endpoint_cmd, 0, sizeof(struct configure_endpoint_command));
              configure_endpoint_cmd.type = 12;
              configure_endpoint_cmd.input_ctx_ptr = V2P(input_ctx);
              configure_endpoint_cmd.slot_id = event->slot_id;

              write_ring(&cmd_ring, &configure_endpoint_cmd);
              write_xhci_doorbell(0, 0);

//              struct input_context *input_ctx2 = (struct input_context *)(P2V(device_contexts[event->slot_id]) - 0x20);
//              xhci_debug("wMaxPacketSize Test:%d\n", input_ctx2->epctx[ep_in_dci].max_packet_size);
            }

            device_status[port_id - 1].state = USB_DEVICE_STATUS_END_OF_INITIALIZE;
          }
        }
      }

//      start = start->next;
    }

    /* Command Completion Event */
    if(trb->type == 33){
      struct command_completion_event *event = (struct command_completion_event *)trb;
      xhci_debug("Command Completion Event code=%s(%d)\n", get_completion_msg(event->code), event->code);
      xhci_debug("Issuer TRB:%s(%d)\n", get_trb_type(((struct normal_trb *)P2V(event->ptr))->type), ((struct normal_trb *)P2V(event->ptr))->type);

//      uint8_t port_id = (uint8_t)(((uint64_t)start - (uint64_t)&device_status[0]) / sizeof(struct device_state)) + 1;
      uint8_t port_id = current_port;
//      xhci_debug("port_id:%d\n", port_id);
//      if(device_contexts[event->slot_id]) xhci_debug("port_id:%d\n", ((struct slot_context *)device_contexts[event->slot_id])->root_hub_port_num);
//      else xhci_debug("port_id:%d\n", current_port);
      if(device_contexts[event->slot_id]) port_id = ((struct slot_context *)P2V(device_contexts[event->slot_id]))->root_hub_port_num;
      xhci_debug("slot_id:%d -> port_id:%d\n", event->slot_id, port_id);

//      xhci_debug("PORTSC(%d):%08X\n", port_id, read_xhci_operation(PORTSC(port_id)));

      /* Enable Slot Command */
//      if(((struct normal_trb *)P2V(event->ptr))->type == 9 && device_status[port_id - 1].state == USB_DEVICE_STATUS_ENABLE_SLOT){
      if(((struct normal_trb *)P2V(event->ptr))->type == 9 && device_status[current_port - 1].state == USB_DEVICE_STATUS_ENABLE_SLOT){
        handle_enable_slot(event, port_id);

        start = start->next;
      }

      /* Address Device Command */
//      if(((struct normal_trb *)P2V(event->ptr))->type == 11 && device_status[port_id - 1].state == USB_DEVICE_STATUS_ADDRESS_DEVICE && !(((struct address_device_command *)P2V(event->ptr))->bsr)){
      if(((struct normal_trb *)P2V(event->ptr))->type == 11 && device_status[current_port - 1].state == USB_DEVICE_STATUS_ADDRESS_DEVICE){
        xhci_debug("Address Device slot:%d\n", event->slot_id);

#if 0
        volatile uint8_t *data_buf = device_status[port_id - 1].buf;
        size_t data_buf_size = sizeof(device_status[port_id - 1].buf);
//        xhci_debug("data_buf:%016X\n", data_buf);
//        xhci_debug("data_buf_size:%d\n", data_buf_size);
#else
        volatile uint8_t *data_buf = device_status[current_port - 1].buf;
        size_t data_buf_size = sizeof(device_status[current_port - 1].buf);
//        xhci_debug("data_buf:%016X\n", data_buf);
//        xhci_debug("data_buf_size:%d\n", data_buf_size);
#endif

        get_descriptor(device_status[port_id - 1].transfer_ring, event->slot_id, DESC_DEVICE, 0, data_buf, data_buf_size);

//        device_status[port_id - 1].state = USB_DEVICE_STATUS_GET_DESCRIPTOR;
        device_status[current_port - 1].state = USB_DEVICE_STATUS_GET_DESCRIPTOR;
#if 1
        printk("current_port reset\n");
        current_port = -1;
        for(int i = 0; i < 256; i++){
          if(device_status[i].state == USB_DEVICE_STATUS_WAIT_ENABLE_SLOT){
            current_port = i + 1;
          }
        }

        printk("current_port:%d\n", current_port);
        if(current_port != -1){
          device_status[current_port - 1].state = USB_DEVICE_STATUS_ENABLE_SLOT;

          struct enable_slot_command enable_slot_cmd;
          memset(&enable_slot_cmd, 0, sizeof(struct enable_slot_command));
          enable_slot_cmd.type = 9;

          write_ring(&cmd_ring, &enable_slot_cmd);
          write_xhci_doorbell(0, 0);
        }
#endif
        start = start->next;
      }
    }

    /* Port Status Change Event */
    if(trb->type == 34){
      struct port_status_change_event *event = (struct port_status_change_event *)trb;

      extern void scroll_screen(void);
      scroll_screen();

      if(read_xhci_operation(PORTSC(event->port) & 0x01)){
        xhci_debug("Configuring port:%d\n", event->port);

        uint32_t portsc = read_xhci_operation(PORTSC(event->port));
        xhci_debug("PORTSC(%d):%08X\n", event->port, portsc);

        /* PED */
        if(portsc & 0x02){
          struct enable_slot_command enable_slot_cmd;
          memset(&enable_slot_cmd, 0, sizeof(struct enable_slot_command));
          enable_slot_cmd.type = 9;

          write_ring(&cmd_ring, &enable_slot_cmd);
          write_xhci_doorbell(0, 0);

#if 1
          if(current_port == -1){
            current_port = event->port;
            device_status[event->port - 1].state = USB_DEVICE_STATUS_ENABLE_SLOT;
          }else{
            device_status[event->port - 1].state = USB_DEVICE_STATUS_WAIT_ENABLE_SLOT;
          }
#else
          if(!start) start = &device_status[event->port - 1];

//          xhci_debug("device_status:%016X\n", &device_status[event->port - 1]);
          device_status[event->port - 1].state = USB_DEVICE_STATUS_ENABLE_SLOT;
          device_status[event->port - 1].next = start;
          if(state_last) state_last->next = &device_status[event->port - 1];

          state_last = &device_status[event->port - 1];
#endif
        }else{
//          uint32_t pls = (portsc >> 5) & 0x0f;
//          xhci_debug("PLS:%d\n", pls);

          portsc &= 0x0e00c3e0;
          portsc |= (1 << 17);  /* Change Connect Status(Clear) */
          /* PORT RESET */
          portsc |= 0x10;
          write_xhci_operation(PORTSC(event->port), portsc);
          xhci_debug("PORTSC(%d):%08X\n", event->port, portsc);
        }
      }
    }
  }
}
