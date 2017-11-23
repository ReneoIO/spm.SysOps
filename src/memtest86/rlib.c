/*
 * Copyright 2016-present Reneo, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -------------------------------------------------------------------------------
 * Author: Minesh B. Amin
 *
 */

#define Assign2Bytes(m_l, m_r)       \
  ({                                 \
     union {                         \
       uint8_t  bytes[2];            \
       uint16_t data;                \
     } u;                            \
                                     \
     u.data = (m_r);                 \
     m_l[0] = u.bytes[0];            \
     m_l[1] = u.bytes[1];            \
  });

#define Assign4Bytes(m_l, m_r)       \
  ({                                 \
     union {                         \
       uint8_t  bytes[4];            \
       uint32_t data;                \
     } u;                            \
                                     \
     u.data = (m_r);                 \
     m_l[0] = u.bytes[0];            \
     m_l[1] = u.bytes[1];            \
     m_l[2] = u.bytes[2];            \
     m_l[3] = u.bytes[3];            \
  });

static void size4KUnits(uint32_t data, uint16_t* data4K, uint8_t *unit) 
{
  /* page is in multiples of 4K */
  if ((data << 2) < 9999) {
    *data4K = (data << 2);
    *unit   = 'K';
  } else if ((data >>  8) < 9999) {
    *data4K = (data  + (1 << 7)) >> 8;
    *unit   = 'M';
  } else if ((data >> 18) < 9999) {
    *data4K = (data  + (1 << 17)) >> 18;
    *unit   = 'G';
  } else {
    *data4K = (data  + (1 << 27)) >> 28;
    *unit   = 'T';
  }
}

static void print_Serial(const int nBytes, const char * p)
{
  int i = 0;
  
  for(i = 0; i < nBytes; i++) {
    WAIT_FOR_XMITR;

    /* Send the character out. */
    serial_echo_outb(*p, UART_TX);
    p++;
  }
}

void halt() 
{
  struct iHalt_t obj = { iHalt_e, };
  
  print_Serial(sizeof(obj), (char*)&obj);
  __asm__("hlt");
}

void print_iErrors(int ecount) 
{
  struct iErrors_t obj = { iErrors_e, };

  Assign4Bytes(obj.ct, ecount);

  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iInterrupt(struct eregs *trap_regs, uint32_t address)
{
  struct iInterrupt_t obj = { iInterrupt_e, };

  Assign4Bytes(obj.vect,  trap_regs->vect);
  Assign4Bytes(obj.eip,   trap_regs->eip);
  Assign4Bytes(obj.cs,    trap_regs->cs);
  Assign4Bytes(obj.eflag, trap_regs->eflag);
  Assign4Bytes(obj.code,  trap_regs->code);
  Assign4Bytes(obj.ds,    trap_regs->ds);
  Assign4Bytes(obj.ss,    trap_regs->ss);
  Assign4Bytes(obj.eax,   trap_regs->eax);
  Assign4Bytes(obj.ebx,   trap_regs->ebx);
  Assign4Bytes(obj.ecx,   trap_regs->ecx);
  Assign4Bytes(obj.edx,   trap_regs->edx);
  Assign4Bytes(obj.edi,   trap_regs->edi);
  Assign4Bytes(obj.esi,   trap_regs->esi);
  Assign4Bytes(obj.ebp,   trap_regs->ebp);
  Assign4Bytes(obj.esp,   trap_regs->esp);
  Assign4Bytes(obj.addr,  address);

  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iIterations(int c_iter) 
{
  struct iIterations_t obj = { iIterations_e, };

  Assign4Bytes(obj.ct, c_iter);
  
  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iMemBytes(uint32_t pages) 
{
  struct iMemBytes_t obj = { iMemBytes_e, };

  {
    uint16_t         size;

    size4KUnits (pages,   &size, &obj.unit);
    Assign2Bytes(obj.size, size);
  }

  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iMemSpeed(uint32_t speed)
{
  struct iMemSpeed_t obj = { iMemSpeed_e, };

  Assign2Bytes(obj.val, speed);

  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iPass(int pass) 
{
  struct iPass_t obj = { iPass_e, };

  if (pass >= 255) {
    obj.ct = 255; // Deadcode (!)
  } else {
    obj.ct = (uint8_t)pass;
  }

  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iPattern(uint32_t pat, int offset) 
{
  struct iPattern_t obj = { iPattern_e, };

  {
    uint16_t        offset_;
    
    if ((offset == -1) || (offset >= 0xffff)) {
      offset_ = 0xffff;
    } else {
      offset_ = (uint16_t)offset;
    }
    
    Assign4Bytes(obj.val,    pat);
    Assign2Bytes(obj.offset, offset_);
  }
  
  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iTest(int seq)
{
  struct iTest_t obj = { iTest_e, };

  {
    if (seq >= 11) {
      obj.seq = 11; // Deadcode (!)
    } else {
      obj.seq = (uint8_t)seq;
    }
  }
  
  print_Serial(sizeof(obj), (char*)&obj);
}

void print_iTestMsg(int p0, int p1, int selected_pages) 
{
  struct iTestMsg_t obj = { iTestMsg_e, };

  {
    uint16_t         size;

    size4KUnits (p0,                &size, &obj.rangeLeft. unit);
    Assign2Bytes(obj.rangeLeft. val, size);
    size4KUnits (p1,                &size, &obj.rangeRight.unit);
    Assign2Bytes(obj.rangeRight.val, size);
    size4KUnits (p1-p0,             &size, &obj.size.      unit);
    Assign2Bytes(obj.size.      val, size);
    size4KUnits (selected_pages,    &size, &obj.sizeMax.   unit);
    Assign2Bytes(obj.sizeMax.   val, size);
  }
  
  print_Serial(sizeof(obj), (char*)&obj);
}

void print_pPass(int pct) 
{
  struct pPass_t obj = { pPass_e, };

  {
    if (pct > 100) {
      obj.pct = 101; // Deadcode (!)
    } else {
      obj.pct = (uint8_t)pct;
    }
  }
  
  print_Serial(sizeof(obj), (char*)&obj);
}

void print_pTest(int pct) 
{
  struct pTest_t obj = { pTest_e, };

  {
    if (pct > 100) {
      obj.pct = 101; // Deadcode (!)
    } else {
      obj.pct = (uint8_t)pct;
    }
  }

  print_Serial(sizeof(obj), (char*)&obj);
}

#undef Assign2Bytes
#undef Assign4Bytes
