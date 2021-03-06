/******************************************************************************
 *
 *  Copyright 2015 Google Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include <base/logging.h>

#include "gd/common/init_flags.h"
#include "hci/include/btsnoop_mem.h"

static btsnoop_data_cb data_callback = NULL;
static activity_attribution_cb attribution_callback = NULL;

void btsnoop_mem_set_callback(btsnoop_data_cb cb) { data_callback = cb; }

void activity_attribution_set_callback(activity_attribution_cb cb) {
  attribution_callback = cb;
}

void btsnoop_mem_capture(const BT_HDR* packet, uint64_t timestamp_us) {
  if (!data_callback && !attribution_callback) return;

  CHECK(packet);

  const uint8_t* data = &packet->data[packet->offset];
  const uint16_t type = packet->event & BT_EVT_MASK;
  size_t length = 0;

  switch (type) {
    case BT_EVT_TO_LM_HCI_CMD:
      if (packet->len > 2) length = data[2] + 3;
      break;

    case BT_EVT_TO_BTU_HCI_EVT:
      if (packet->len > 1) length = data[1] + 2;
      break;

    case BT_EVT_TO_LM_HCI_ACL:
    case BT_EVT_TO_BTU_HCI_ACL:
      if (packet->len > 3) length = (data[2] | (data[3] << 8)) + 4;
      break;

    case BT_EVT_TO_LM_HCI_SCO:
    case BT_EVT_TO_BTU_HCI_SCO:
      if (packet->len > 2) length = data[2] + 3;
      break;

    case BT_EVT_TO_LM_HCI_ISO:
    case BT_EVT_TO_BTU_HCI_ISO:
      if (packet->len > 3) length = (data[2] | ((data[3] & 0x3f) << 8)) + 4;
      break;
  }

  if (length && data_callback)
    (*data_callback)(type, data, length, timestamp_us);
  if (length && attribution_callback &&
      bluetooth::common::init_flags::btaa_hci_is_enabled()) {
    (*attribution_callback)(type, data, length, timestamp_us);
  }
}
