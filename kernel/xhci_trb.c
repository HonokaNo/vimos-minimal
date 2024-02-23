#include <string.h>
#include "xhci.h"

void init_normal_trb(struct normal_trb *trb, uint64_t buf){
  memset(trb, 0, sizeof(struct normal_trb));
  trb->type = 1;
  trb->buffer = buf;
  trb->ioc = 1;
}

void init_setup_stage_trb(struct setup_stage_trb *trb){
  memset(trb, 0, sizeof(struct setup_stage_trb));
  trb->type = 2;
  trb->idt = 1;
  trb->trb_length = 8;
}

void init_data_stage_trb(struct data_stage_trb *trb, uint64_t buf, uint32_t len){
  memset(trb, 0, sizeof(struct data_stage_trb));
  trb->type = 3;
  trb->data_buf = buf;
  trb->length = len;
  trb->ioc = 1;
}

void init_status_stage_trb(struct status_stage_trb *trb){
  memset(trb, 0, sizeof(struct status_stage_trb));
  trb->type = 4;
}

void init_address_device_command(struct address_device_command *trb, uint64_t ctx, uint8_t slot){
  memset(trb, 0, sizeof(struct address_device_command));
  trb->type = 11;
  trb->input_ctx_ptr = ctx;
  trb->slot_id = slot;
}
