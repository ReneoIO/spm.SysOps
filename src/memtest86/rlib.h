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

#ifndef _RLIB_H
#define _RLIB_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

// --------------------------------------------------------------------------------
// Nullified API ...

#define check_input()
#define clear_scroll()
#define footer()
#define get_key() 0
#define scroll()
#define wait_keyup()

#define get_config()
#define pop2clear()
#define pop2down()
#define pop2up()
#define popclear()
#define popdown()
#define popup()

#define display_init()

#define aprint( m_arg1, m_arg2, m_arg3)
#define cplace( m_arg1, m_arg2, m_arg3)
#define cprint( m_arg1, m_arg2, m_arg3)
#define dprint( m_arg1, m_arg2, m_arg3, m_arg4, m_arg5)
#define hprint( m_arg1, m_arg2, m_arg3)
#define hprint2(m_arg1, m_arg2, m_arg3, m_arg4)
#define hprint3(m_arg1, m_arg2, m_arg3, m_arg4)
#define xprint( m_arg1, m_arg2, m_arg3)

#define btrace( m_arg1, m_arg2, m_arg3, m_arg4, m_arg5, m_arg6)

// --------------------------------------------------------------------------------
// New serial print API ...

struct eregs; // Forward declaration (!)

void halt             ();
void print_iErrors    (int           ecount);
void print_iInterrupt (struct eregs *trap_regs, uint32_t address);
void print_iIterations(int           c_iter);
void print_iMemBytes  (uint32_t      pages);
void print_iMemSpeed  (uint32_t      speed);
void print_iPass      (int           pass);
void print_iPattern   (uint32_t      pat,       int      offset);
void print_iTest      (int           seq);
void print_iTestMsg   (int           p0,        int      p1, int selected_pages);
void print_pPass      (int           pct);
void print_pTest      (int           pct);

// --------------------------------------------------------------------------------
// Payload structures ...

#define iErrors_e      0x13
struct  iErrors_t {
  uint8_t id;
  uint8_t ct[4];
};

#define iHalt_e       (iErrors_e     + 1)
struct  iHalt_t {
  uint8_t id;
};

#define iInterrupt_e  (iHalt_e       + 1)
struct  iInterrupt_t {
  uint8_t id;
  uint8_t vect [4];
  uint8_t eip  [4];
  uint8_t cs   [4];
  uint8_t eflag[4];
  uint8_t code [4];
  uint8_t ds   [4];
  uint8_t ss   [4];
  uint8_t addr [4];
  uint8_t eax  [4];
  uint8_t ebx  [4];
  uint8_t ecx  [4];
  uint8_t edx  [4];
  uint8_t edi  [4];
  uint8_t esi  [4];
  uint8_t ebp  [4];
  uint8_t esp  [4];
};

#define iIterations_e (iInterrupt_e  + 1)
struct  iIterations_t {
  uint8_t id;
  uint8_t ct[4];
};

#define iMemBytes_e   (iIterations_e + 1)
struct  iMemBytes_t {
  uint8_t id;
  uint8_t size[2];
  uint8_t unit;
};

#define iMemSpeed_e   (iMemBytes_e   + 1)
struct  iMemSpeed_t {
  uint8_t id;
  uint8_t val[4];
};

#define iPass_e       (iMemSpeed_e   + 1)
struct  iPass_t {
  uint8_t id;
  uint8_t ct;
};

#define iPattern_e    (iPass_e       + 1)
struct  iPattern_t {
  uint8_t id;
  uint8_t val   [4];
  uint8_t offset[2];
};

#define iTest_e       (iPattern_e    + 1)
struct  iTest_t {
  uint8_t id;
  uint8_t seq;       // 0 .. 10 (i.e. tSeq_00 ... tSeq_10)
};

#define iTestMsg_e    (iTest_e       + 1)
struct  iTestMsg_t {
  uint8_t   id;
  struct iMem_t {
    uint8_t val[2];
    uint8_t unit;
  }         rangeLeft,
            rangeRight,
            size,
            sizeMax;
};

#define pPass_e       (iTestMsg_e    + 1)
struct  pPass_t {
  uint8_t id;
  uint8_t pct;
};

#define pTest_e       (pPass_e       + 1)
struct  pTest_t {
  uint8_t id;
  uint8_t pct;
};

// --------------------------------------------------------------------------------

#define tSeq_00 "[Address test, walking ones, no cache] "
#define tSeq_01 "[Address test, own address Sequential] "
#define tSeq_02 "[Address test, own address Parallel]   "
#define tSeq_03 "[Moving inversions, 1s & 0s Parallel]  "
#define tSeq_04 "[Moving inversions, 8 bit pattern]     "
#define tSeq_05 "[Moving inversions, random pattern]    "
#define tSeq_06 "[Block move]                           "
#define tSeq_07 "[Moving inversions, 32 bit pattern]    "
#define tSeq_08 "[Random number sequence]               "
#define tSeq_09 "[Modulo 20, Random pattern]            "
#define tSeq_10 "[Bit fade test, 2 patterns]            "
#define tSeq_11 "**/-@-/**                              "

#define iInt_00 "  Divide"
#define iInt_01 "   Debug"
#define iInt_02 "     NMI"
#define iInt_03 "  Brkpnt"
#define iInt_04 "Overflow"
#define iInt_05 "   Bound"
#define iInt_06 "  Inv_Op"
#define iInt_07 " No_Math"
#define iInt_08 "Double_Fault"
#define iInt_09 "Seg_Over"
#define iInt_10 " Inv_TSS"
#define iInt_11 "  Seg_NP"
#define iInt_12 "Stack_Fault"
#define iInt_13 "Gen_Prot"
#define iInt_14 "Page_Fault"
#define iInt_15 "   Resvd"
#define iInt_16 "     FPE"
#define iInt_17 "Alignment"
#define iInt_18 " Mch_Chk"
#define iInt_19 "SIMD FPE"
#define iInt_20 "**/-@-/**"

// --------------------------------------------------------------------------------

#endif /* !_RLIB_H */
