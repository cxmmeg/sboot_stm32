/* This file is the part of the STM32 secure bootloader
 *
 * RC5-32/12/128-CBC block cipher implementation based on
 * Ronald L. Rivest "The RC5 Encryption Algorithm"
 * http://people.csail.mit.edu/rivest/Rivest-rc5rev.pdf
 * 
 * Copyright ©2016 Dmitry Filimonchuk <dmitrystu[at]gmail[dot]com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include <stdint.h>
#include "rot.h"
#include "rc5.h"
#include "../config.h"

#define rounds      12
#define c           4
#define t           2 * (rounds + 1)


#define Pw          0xb7e15163
#define Qw          0x9e3779b9


inline static void __memcpy(void *dst, const void *src, uint32_t sz) {
    do {
        *(uint8_t*)dst++ = *(uint8_t*)src++;
    } while (--sz);
}

static uint32_t rc5_keys[t];
static uint32_t CK[2];



static void rc5_encode_block (uint32_t *out, const uint32_t *in) {
    uint32_t A = (in[0] ^ CK[0]) + rc5_keys[0];
    uint32_t B = (in[1] ^ CK[1]) + rc5_keys[1];
    for (int i = 1; i <= rounds; i++) {
        A = __rol((A ^ B), B) + rc5_keys[2 * i];
        B = __rol((B ^ A), A) + rc5_keys[2 * i + 1];
    }
    CK[0] = out[0] = A;
    CK[1] = out[1] = B;
}

static void rc5_decode_block (uint32_t *out, const uint32_t *in) {
    uint32_t A = in[0];
    uint32_t B = in[1];
    uint32_t i0 = A;
    uint32_t i1 = B;
    for (int i = rounds; i > 0; i--) {
        B = __ror((B - rc5_keys[2 * i + 1]), A) ^ A;
        A = __ror((A - rc5_keys[2 * i]), B) ^ B;
    }
    out[0] = (A - rc5_keys[0]) ^ CK[0];
    out[1] = (B - rc5_keys[1]) ^ CK[1];
    CK[0] = i0;
    CK[1] = i1;
}

void rc5_init (const uint8_t *key) {
    uint32_t L[4];
    __memcpy(L, key, 16);
    rc5_keys[0] = Pw;
    for (int i = 1; i < t; i++){
        rc5_keys[i] = rc5_keys[i-1] + Qw;
    }
    for (uint32_t A = 0, B = 0, i = 0, j = 0, k = 3 * t; k > 0; k--) {
        A = rc5_keys[i] = __rol(rc5_keys[i] + A + B, 3);
        B = L[j] = __rol(L[j] + A + B, (A + B));
        if (++i == t) i = 0;
        if (++j == c) j = 0;
    }
    CK[0] = DFU_AES_NONCE0;
    CK[1] = DFU_AES_NONCE1;
}

void rc5_encrypt(uint32_t *out, const uint32_t *in, int32_t bytes) {
    while (bytes > 0) {
        rc5_encode_block(out, in);
        out += 2;
        in += 2;
        bytes -=8;
    }
}

void rc5_decrypt(uint32_t *out, const uint32_t *in, int32_t bytes) {
    while (bytes > 0) {
        rc5_decode_block(out, in);
        out += 2;
        in += 2;
        bytes -= 0x08;
    }
}






