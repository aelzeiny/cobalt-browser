// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <stdio.h>

#include "crypto/p224.h"

#include "testing/gtest/include/gtest/gtest.h"

using namespace crypto;
using p224::Point;

// kBasePointExternal is the P224 base point in external representation.
static const uint8 kBasePointExternal[56] = {
  0xb7, 0x0e, 0x0c, 0xbd, 0x6b, 0xb4, 0xbf, 0x7f,
  0x32, 0x13, 0x90, 0xb9, 0x4a, 0x03, 0xc1, 0xd3,
  0x56, 0xc2, 0x11, 0x22, 0x34, 0x32, 0x80, 0xd6,
  0x11, 0x5c, 0x1d, 0x21, 0xbd, 0x37, 0x63, 0x88,
  0xb5, 0xf7, 0x23, 0xfb, 0x4c, 0x22, 0xdf, 0xe6,
  0xcd, 0x43, 0x75, 0xa0, 0x5a, 0x07, 0x47, 0x64,
  0x44, 0xd5, 0x81, 0x99, 0x85, 0x00, 0x7e, 0x34,
};

// TestVector represents a test of scalar multiplication of the base point.
// |scalar| is a big-endian scalar and |affine| is the external representation
// of g*scalar.
struct TestVector {
  uint8 scalar[28];
  uint8 affine[28*2];
};

static const int kNumNISTTestVectors = 52;

// kNISTTestVectors are the NIST test vectors for P224.
static const TestVector kNISTTestVectors[kNumNISTTestVectors] = {
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x01},
    {0xb7, 0x0e, 0x0c, 0xbd, 0x6b, 0xb4, 0xbf, 0x7f,
     0x32, 0x13, 0x90, 0xb9, 0x4a, 0x03, 0xc1, 0xd3,
     0x56, 0xc2, 0x11, 0x22, 0x34, 0x32, 0x80, 0xd6,
     0x11, 0x5c, 0x1d, 0x21, 0xbd, 0x37, 0x63, 0x88,
     0xb5, 0xf7, 0x23, 0xfb, 0x4c, 0x22, 0xdf, 0xe6,
     0xcd, 0x43, 0x75, 0xa0, 0x5a, 0x07, 0x47, 0x64,
     0x44, 0xd5, 0x81, 0x99, 0x85, 0x00, 0x7e, 0x34
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x02, },

    {0x70, 0x6a, 0x46, 0xdc, 0x76, 0xdc, 0xb7, 0x67,
     0x98, 0xe6, 0x0e, 0x6d, 0x89, 0x47, 0x47, 0x88,
     0xd1, 0x6d, 0xc1, 0x80, 0x32, 0xd2, 0x68, 0xfd,
     0x1a, 0x70, 0x4f, 0xa6, 0x1c, 0x2b, 0x76, 0xa7,
     0xbc, 0x25, 0xe7, 0x70, 0x2a, 0x70, 0x4f, 0xa9,
     0x86, 0x89, 0x28, 0x49, 0xfc, 0xa6, 0x29, 0x48,
     0x7a, 0xcf, 0x37, 0x09, 0xd2, 0xe4, 0xe8, 0xbb,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x03, },
    {0xdf, 0x1b, 0x1d, 0x66, 0xa5, 0x51, 0xd0, 0xd3,
     0x1e, 0xff, 0x82, 0x25, 0x58, 0xb9, 0xd2, 0xcc,
     0x75, 0xc2, 0x18, 0x02, 0x79, 0xfe, 0x0d, 0x08,
     0xfd, 0x89, 0x6d, 0x04, 0xa3, 0xf7, 0xf0, 0x3c,
     0xad, 0xd0, 0xbe, 0x44, 0x4c, 0x0a, 0xa5, 0x68,
     0x30, 0x13, 0x0d, 0xdf, 0x77, 0xd3, 0x17, 0x34,
     0x4e, 0x1a, 0xf3, 0x59, 0x19, 0x81, 0xa9, 0x25,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x04, },
    {0xae, 0x99, 0xfe, 0xeb, 0xb5, 0xd2, 0x69, 0x45,
     0xb5, 0x48, 0x92, 0x09, 0x2a, 0x8a, 0xee, 0x02,
     0x91, 0x29, 0x30, 0xfa, 0x41, 0xcd, 0x11, 0x4e,
     0x40, 0x44, 0x73, 0x01, 0x04, 0x82, 0x58, 0x0a,
     0x0e, 0xc5, 0xbc, 0x47, 0xe8, 0x8b, 0xc8, 0xc3,
     0x78, 0x63, 0x2c, 0xd1, 0x96, 0xcb, 0x3f, 0xa0,
     0x58, 0xa7, 0x11, 0x4e, 0xb0, 0x30, 0x54, 0xc9,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x05, },
    {0x31, 0xc4, 0x9a, 0xe7, 0x5b, 0xce, 0x78, 0x07,
     0xcd, 0xff, 0x22, 0x05, 0x5d, 0x94, 0xee, 0x90,
     0x21, 0xfe, 0xdb, 0xb5, 0xab, 0x51, 0xc5, 0x75,
     0x26, 0xf0, 0x11, 0xaa, 0x27, 0xe8, 0xbf, 0xf1,
     0x74, 0x56, 0x35, 0xec, 0x5b, 0xa0, 0xc9, 0xf1,
     0xc2, 0xed, 0xe1, 0x54, 0x14, 0xc6, 0x50, 0x7d,
     0x29, 0xff, 0xe3, 0x7e, 0x79, 0x0a, 0x07, 0x9b,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x06, },
    {0x1f, 0x24, 0x83, 0xf8, 0x25, 0x72, 0x25, 0x1f,
     0xca, 0x97, 0x5f, 0xea, 0x40, 0xdb, 0x82, 0x1d,
     0xf8, 0xad, 0x82, 0xa3, 0xc0, 0x02, 0xee, 0x6c,
     0x57, 0x11, 0x24, 0x08, 0x89, 0xfa, 0xf0, 0xcc,
     0xb7, 0x50, 0xd9, 0x9b, 0x55, 0x3c, 0x57, 0x4f,
     0xad, 0x7e, 0xcf, 0xb0, 0x43, 0x85, 0x86, 0xeb,
     0x39, 0x52, 0xaf, 0x5b, 0x4b, 0x15, 0x3c, 0x7e,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x07, },
    {0xdb, 0x2f, 0x6b, 0xe6, 0x30, 0xe2, 0x46, 0xa5,
     0xcf, 0x7d, 0x99, 0xb8, 0x51, 0x94, 0xb1, 0x23,
     0xd4, 0x87, 0xe2, 0xd4, 0x66, 0xb9, 0x4b, 0x24,
     0xa0, 0x3c, 0x3e, 0x28, 0x0f, 0x3a, 0x30, 0x08,
     0x54, 0x97, 0xf2, 0xf6, 0x11, 0xee, 0x25, 0x17,
     0xb1, 0x63, 0xef, 0x8c, 0x53, 0xb7, 0x15, 0xd1,
     0x8b, 0xb4, 0xe4, 0x80, 0x8d, 0x02, 0xb9, 0x63,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x08, },
    {0x85, 0x8e, 0x6f, 0x9c, 0xc6, 0xc1, 0x2c, 0x31,
     0xf5, 0xdf, 0x12, 0x4a, 0xa7, 0x77, 0x67, 0xb0,
     0x5c, 0x8b, 0xc0, 0x21, 0xbd, 0x68, 0x3d, 0x2b,
     0x55, 0x57, 0x15, 0x50, 0x04, 0x6d, 0xcd, 0x3e,
     0xa5, 0xc4, 0x38, 0x98, 0xc5, 0xc5, 0xfc, 0x4f,
     0xda, 0xc7, 0xdb, 0x39, 0xc2, 0xf0, 0x2e, 0xbe,
     0xe4, 0xe3, 0x54, 0x1d, 0x1e, 0x78, 0x04, 0x7a,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x09, },
    {0x2f, 0xdc, 0xcc, 0xfe, 0xe7, 0x20, 0xa7, 0x7e,
     0xf6, 0xcb, 0x3b, 0xfb, 0xb4, 0x47, 0xf9, 0x38,
     0x31, 0x17, 0xe3, 0xda, 0xa4, 0xa0, 0x7e, 0x36,
     0xed, 0x15, 0xf7, 0x8d, 0x37, 0x17, 0x32, 0xe4,
     0xf4, 0x1b, 0xf4, 0xf7, 0x88, 0x30, 0x35, 0xe6,
     0xa7, 0x9f, 0xce, 0xdc, 0x0e, 0x19, 0x6e, 0xb0,
     0x7b, 0x48, 0x17, 0x16, 0x97, 0x51, 0x74, 0x63,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x0a, },
    {0xae, 0xa9, 0xe1, 0x7a, 0x30, 0x65, 0x17, 0xeb,
     0x89, 0x15, 0x2a, 0xa7, 0x09, 0x6d, 0x2c, 0x38,
     0x1e, 0xc8, 0x13, 0xc5, 0x1a, 0xa8, 0x80, 0xe7,
     0xbe, 0xe2, 0xc0, 0xfd, 0x39, 0xbb, 0x30, 0xea,
     0xb3, 0x37, 0xe0, 0xa5, 0x21, 0xb6, 0xcb, 0xa1,
     0xab, 0xe4, 0xb2, 0xb3, 0xa3, 0xe5, 0x24, 0xc1,
     0x4a, 0x3f, 0xe3, 0xeb, 0x11, 0x6b, 0x65, 0x5f,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x0b, },
    {0xef, 0x53, 0xb6, 0x29, 0x4a, 0xca, 0x43, 0x1f,
     0x0f, 0x3c, 0x22, 0xdc, 0x82, 0xeb, 0x90, 0x50,
     0x32, 0x4f, 0x1d, 0x88, 0xd3, 0x77, 0xe7, 0x16,
     0x44, 0x8e, 0x50, 0x7c, 0x20, 0xb5, 0x10, 0x00,
     0x40, 0x92, 0xe9, 0x66, 0x36, 0xcf, 0xb7, 0xe3,
     0x2e, 0xfd, 0xed, 0x82, 0x65, 0xc2, 0x66, 0xdf,
     0xb7, 0x54, 0xfa, 0x6d, 0x64, 0x91, 0xa6, 0xda,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x0c, },
    {0x6e, 0x31, 0xee, 0x1d, 0xc1, 0x37, 0xf8, 0x1b,
     0x05, 0x67, 0x52, 0xe4, 0xde, 0xab, 0x14, 0x43,
     0xa4, 0x81, 0x03, 0x3e, 0x9b, 0x4c, 0x93, 0xa3,
     0x04, 0x4f, 0x4f, 0x7a, 0x20, 0x7d, 0xdd, 0xf0,
     0x38, 0x5b, 0xfd, 0xea, 0xb6, 0xe9, 0xac, 0xda,
     0x8d, 0xa0, 0x6b, 0x3b, 0xbe, 0xf2, 0x24, 0xa9,
     0x3a, 0xb1, 0xe9, 0xe0, 0x36, 0x10, 0x9d, 0x13,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x0d, },
    {0x34, 0xe8, 0xe1, 0x7a, 0x43, 0x0e, 0x43, 0x28,
     0x97, 0x93, 0xc3, 0x83, 0xfa, 0xc9, 0x77, 0x42,
     0x47, 0xb4, 0x0e, 0x9e, 0xbd, 0x33, 0x66, 0x98,
     0x1f, 0xcf, 0xae, 0xca, 0x25, 0x28, 0x19, 0xf7,
     0x1c, 0x7f, 0xb7, 0xfb, 0xcb, 0x15, 0x9b, 0xe3,
     0x37, 0xd3, 0x7d, 0x33, 0x36, 0xd7, 0xfe, 0xb9,
     0x63, 0x72, 0x4f, 0xdf, 0xb0, 0xec, 0xb7, 0x67,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x0e, },
    {0xa5, 0x36, 0x40, 0xc8, 0x3d, 0xc2, 0x08, 0x60,
     0x3d, 0xed, 0x83, 0xe4, 0xec, 0xf7, 0x58, 0xf2,
     0x4c, 0x35, 0x7d, 0x7c, 0xf4, 0x80, 0x88, 0xb2,
     0xce, 0x01, 0xe9, 0xfa, 0xd5, 0x81, 0x4c, 0xd7,
     0x24, 0x19, 0x9c, 0x4a, 0x5b, 0x97, 0x4a, 0x43,
     0x68, 0x5f, 0xbf, 0x5b, 0x8b, 0xac, 0x69, 0x45,
     0x9c, 0x94, 0x69, 0xbc, 0x8f, 0x23, 0xcc, 0xaf,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x0f, },
    {0xba, 0xa4, 0xd8, 0x63, 0x55, 0x11, 0xa7, 0xd2,
     0x88, 0xae, 0xbe, 0xed, 0xd1, 0x2c, 0xe5, 0x29,
     0xff, 0x10, 0x2c, 0x91, 0xf9, 0x7f, 0x86, 0x7e,
     0x21, 0x91, 0x6b, 0xf9, 0x97, 0x9a, 0x5f, 0x47,
     0x59, 0xf8, 0x0f, 0x4f, 0xb4, 0xec, 0x2e, 0x34,
     0xf5, 0x56, 0x6d, 0x59, 0x56, 0x80, 0xa1, 0x17,
     0x35, 0xe7, 0xb6, 0x10, 0x46, 0x12, 0x79, 0x89,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x10, },
    {0x0b, 0x6e, 0xc4, 0xfe, 0x17, 0x77, 0x38, 0x24,
     0x04, 0xef, 0x67, 0x99, 0x97, 0xba, 0x8d, 0x1c,
     0xc5, 0xcd, 0x8e, 0x85, 0x34, 0x92, 0x59, 0xf5,
     0x90, 0xc4, 0xc6, 0x6d, 0x33, 0x99, 0xd4, 0x64,
     0x34, 0x59, 0x06, 0xb1, 0x1b, 0x00, 0xe3, 0x63,
     0xef, 0x42, 0x92, 0x21, 0xf2, 0xec, 0x72, 0x0d,
     0x2f, 0x66, 0x5d, 0x7d, 0xea, 0xd5, 0xb4, 0x82,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x11, },
    {0xb8, 0x35, 0x7c, 0x3a, 0x6c, 0xee, 0xf2, 0x88,
     0x31, 0x0e, 0x17, 0xb8, 0xbf, 0xef, 0xf9, 0x20,
     0x08, 0x46, 0xca, 0x8c, 0x19, 0x42, 0x49, 0x7c,
     0x48, 0x44, 0x03, 0xbc, 0xff, 0x14, 0x9e, 0xfa,
     0x66, 0x06, 0xa6, 0xbd, 0x20, 0xef, 0x7d, 0x1b,
     0x06, 0xbd, 0x92, 0xf6, 0x90, 0x46, 0x39, 0xdc,
     0xe5, 0x17, 0x4d, 0xb6, 0xcc, 0x55, 0x4a, 0x26,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x12, },
    {0xc9, 0xff, 0x61, 0xb0, 0x40, 0x87, 0x4c, 0x05,
     0x68, 0x47, 0x92, 0x16, 0x82, 0x4a, 0x15, 0xea,
     0xb1, 0xa8, 0x38, 0xa7, 0x97, 0xd1, 0x89, 0x74,
     0x62, 0x26, 0xe4, 0xcc, 0xea, 0x98, 0xd6, 0x0e,
     0x5f, 0xfc, 0x9b, 0x8f, 0xcf, 0x99, 0x9f, 0xab,
     0x1d, 0xf7, 0xe7, 0xef, 0x70, 0x84, 0xf2, 0x0d,
     0xdb, 0x61, 0xbb, 0x04, 0x5a, 0x6c, 0xe0, 0x02,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x13, },
    {0xa1, 0xe8, 0x1c, 0x04, 0xf3, 0x0c, 0xe2, 0x01,
     0xc7, 0xc9, 0xac, 0xe7, 0x85, 0xed, 0x44, 0xcc,
     0x33, 0xb4, 0x55, 0xa0, 0x22, 0xf2, 0xac, 0xdb,
     0xc6, 0xca, 0xe8, 0x3c, 0xdc, 0xf1, 0xf6, 0xc3,
     0xdb, 0x09, 0xc7, 0x0a, 0xcc, 0x25, 0x39, 0x1d,
     0x49, 0x2f, 0xe2, 0x5b, 0x4a, 0x18, 0x0b, 0xab,
     0xd6, 0xce, 0xa3, 0x56, 0xc0, 0x47, 0x19, 0xcd,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x14, },
    {0xfc, 0xc7, 0xf2, 0xb4, 0x5d, 0xf1, 0xcd, 0x5a,
     0x3c, 0x0c, 0x07, 0x31, 0xca, 0x47, 0xa8, 0xaf,
     0x75, 0xcf, 0xb0, 0x34, 0x7e, 0x83, 0x54, 0xee,
     0xfe, 0x78, 0x24, 0x55, 0x0d, 0x5d, 0x71, 0x10,
     0x27, 0x4c, 0xba, 0x7c, 0xde, 0xe9, 0x0e, 0x1a,
     0x8b, 0x0d, 0x39, 0x4c, 0x37, 0x6a, 0x55, 0x73,
     0xdb, 0x6b, 0xe0, 0xbf, 0x27, 0x47, 0xf5, 0x30,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x01, 0x8e, 0xbb, 0xb9,
     0x5e, 0xed, 0x0e, 0x13, },
    {0x61, 0xf0, 0x77, 0xc6, 0xf6, 0x2e, 0xd8, 0x02,
     0xda, 0xd7, 0xc2, 0xf3, 0x8f, 0x5c, 0x67, 0xf2,
     0xcc, 0x45, 0x36, 0x01, 0xe6, 0x1b, 0xd0, 0x76,
     0xbb, 0x46, 0x17, 0x9e, 0x22, 0x72, 0xf9, 0xe9,
     0xf5, 0x93, 0x3e, 0x70, 0x38, 0x8e, 0xe6, 0x52,
     0x51, 0x34, 0x43, 0xb5, 0xe2, 0x89, 0xdd, 0x13,
     0x5d, 0xcc, 0x0d, 0x02, 0x99, 0xb2, 0x25, 0xe4,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x9d, 0x89,
     0x3d, 0x4c, 0xdd, 0x74, 0x72, 0x46, 0xcd, 0xca,
     0x43, 0x59, 0x0e, 0x13, },
    {0x02, 0x98, 0x95, 0xf0, 0xaf, 0x49, 0x6b, 0xfc,
     0x62, 0xb6, 0xef, 0x8d, 0x8a, 0x65, 0xc8, 0x8c,
     0x61, 0x39, 0x49, 0xb0, 0x36, 0x68, 0xaa, 0xb4,
     0xf0, 0x42, 0x9e, 0x35, 0x3e, 0xa6, 0xe5, 0x3f,
     0x9a, 0x84, 0x1f, 0x20, 0x19, 0xec, 0x24, 0xbd,
     0xe1, 0xa7, 0x56, 0x77, 0xaa, 0x9b, 0x59, 0x02,
     0xe6, 0x10, 0x81, 0xc0, 0x10, 0x64, 0xde, 0x93,
    },
  },
  {
    {0x41, 0xff, 0xc1, 0xff, 0xff, 0xfe, 0x01, 0xff,
     0xfc, 0x00, 0x03, 0xff, 0xfe, 0x00, 0x07, 0xc0,
     0x01, 0xff, 0xf0, 0x00, 0x03, 0xff, 0xf0, 0x7f,
     0xfe, 0x00, 0x07, 0xc0, },
    {0xab, 0x68, 0x99, 0x30, 0xbc, 0xae, 0x4a, 0x4a,
     0xa5, 0xf5, 0xcb, 0x08, 0x5e, 0x82, 0x3e, 0x8a,
     0xe3, 0x0f, 0xd3, 0x65, 0xeb, 0x1d, 0xa4, 0xab,
     0xa9, 0xcf, 0x03, 0x79, 0x33, 0x45, 0xa1, 0x21,
     0xbb, 0xd2, 0x33, 0x54, 0x8a, 0xf0, 0xd2, 0x10,
     0x65, 0x4e, 0xb4, 0x0b, 0xab, 0x78, 0x8a, 0x03,
     0x66, 0x64, 0x19, 0xbe, 0x6f, 0xbd, 0x34, 0xe7,
    },
  },
  {
    {0x7f, 0xff, 0xff, 0xc0, 0x3f, 0xff, 0xc0, 0x03,
     0xff, 0xff, 0xfc, 0x00, 0x7f, 0xff, 0x00, 0x00,
     0x00, 0x00, 0x07, 0x00, 0x00, 0x10, 0x00, 0x00,
     0x00, 0x0e, 0x00, 0xff, },
    {0xbd, 0xb6, 0xa8, 0x81, 0x7c, 0x1f, 0x89, 0xda,
     0x1c, 0x2f, 0x3d, 0xd8, 0xe9, 0x7f, 0xeb, 0x44,
     0x94, 0xf2, 0xed, 0x30, 0x2a, 0x4c, 0xe2, 0xbc,
     0x7f, 0x5f, 0x40, 0x25, 0x4c, 0x70, 0x20, 0xd5,
     0x7c, 0x00, 0x41, 0x18, 0x89, 0x46, 0x2d, 0x77,
     0xa5, 0x43, 0x8b, 0xb4, 0xe9, 0x7d, 0x17, 0x77,
     0x00, 0xbf, 0x72, 0x43, 0xa0, 0x7f, 0x16, 0x80,
    },
  },
  {
    {0x7f, 0xff, 0xff, 0x04, 0x00, 0x00, 0x00, 0x00,
     0xff, 0xff, 0xf0, 0x1f, 0xff, 0xf8, 0xff, 0xff,
     0xc0, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xc0, 0x00,
     0x00, 0x0f, 0xff, 0xff, },
    {0xd5, 0x8b, 0x61, 0xaa, 0x41, 0xc3, 0x2d, 0xd5,
     0xeb, 0xa4, 0x62, 0x64, 0x7d, 0xba, 0x75, 0xc5,
     0xd6, 0x7c, 0x83, 0x60, 0x6c, 0x0a, 0xf2, 0xbd,
     0x92, 0x84, 0x46, 0xa9, 0xd2, 0x4b, 0xa6, 0xa8,
     0x37, 0xbe, 0x04, 0x60, 0xdd, 0x10, 0x7a, 0xe7,
     0x77, 0x25, 0x69, 0x6d, 0x21, 0x14, 0x46, 0xc5,
     0x60, 0x9b, 0x45, 0x95, 0x97, 0x6b, 0x16, 0xbd,
    },
  },
  {
    {0x7f, 0xff, 0xff, 0xc0, 0x00, 0xff, 0xfe, 0x3f,
     0xff, 0xfc, 0x10, 0x00, 0x00, 0x20, 0x00, 0x3f,
     0xff, 0xff, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00,
     0x3f, 0xff, 0xff, 0xff, },
    {0xdc, 0x9f, 0xa7, 0x79, 0x78, 0xa0, 0x05, 0x51,
     0x09, 0x80, 0xe9, 0x29, 0xa1, 0x48, 0x5f, 0x63,
     0x71, 0x6d, 0xf6, 0x95, 0xd7, 0xa0, 0xc1, 0x8b,
     0xb5, 0x18, 0xdf, 0x03, 0xed, 0xe2, 0xb0, 0x16,
     0xf2, 0xdd, 0xff, 0xc2, 0xa8, 0xc0, 0x15, 0xb1,
     0x34, 0x92, 0x82, 0x75, 0xce, 0x09, 0xe5, 0x66,
     0x1b, 0x7a, 0xb1, 0x4c, 0xe0, 0xd1, 0xd4, 0x03,
    },
  },
  {
    {0x70, 0x01, 0xf0, 0x00, 0x1c, 0x00, 0x01, 0xc0,
     0x00, 0x00, 0x1f, 0xff, 0xff, 0xfc, 0x00, 0x00,
     0x1f, 0xff, 0xff, 0xf8, 0x00, 0x0f, 0xc0, 0x00,
     0x00, 0x01, 0xfc, 0x00, },
    {0x49, 0x9d, 0x8b, 0x28, 0x29, 0xcf, 0xb8, 0x79,
     0xc9, 0x01, 0xf7, 0xd8, 0x5d, 0x35, 0x70, 0x45,
     0xed, 0xab, 0x55, 0x02, 0x88, 0x24, 0xd0, 0xf0,
     0x5b, 0xa2, 0x79, 0xba, 0xbf, 0x92, 0x95, 0x37,
     0xb0, 0x6e, 0x40, 0x15, 0x91, 0x96, 0x39, 0xd9,
     0x4f, 0x57, 0x83, 0x8f, 0xa3, 0x3f, 0xc3, 0xd9,
     0x52, 0x59, 0x8d, 0xcd, 0xbb, 0x44, 0xd6, 0x38,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x00, 0x1f, 0xfc, 0x00, 0x00,
     0x00, 0xff, 0xf0, 0x30, 0x00, 0x1f, 0x00, 0x00,
     0xff, 0xff, 0xf0, 0x00, 0x00, 0x38, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x02, },
    {0x82, 0x46, 0xc9, 0x99, 0x13, 0x71, 0x86, 0x63,
     0x2c, 0x5f, 0x9e, 0xdd, 0xf3, 0xb1, 0xb0, 0xe1,
     0x76, 0x4c, 0x5e, 0x8b, 0xd0, 0xe0, 0xd8, 0xa5,
     0x54, 0xb9, 0xcb, 0x77, 0xe8, 0x0e, 0xd8, 0x66,
     0x0b, 0xc1, 0xcb, 0x17, 0xac, 0x7d, 0x84, 0x5b,
     0xe4, 0x0a, 0x7a, 0x02, 0x2d, 0x33, 0x06, 0xf1,
     0x16, 0xae, 0x9f, 0x81, 0xfe, 0xa6, 0x59, 0x47,
    },
  },
  {
    {0x7f, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x07, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0xff, 0xfe, 0x08, 0x00, 0x00, 0x1f,
     0xf0, 0x00, 0x1f, 0xff, },
    {0x66, 0x70, 0xc2, 0x0a, 0xfc, 0xce, 0xae, 0xa6,
     0x72, 0xc9, 0x7f, 0x75, 0xe2, 0xe9, 0xdd, 0x5c,
     0x84, 0x60, 0xe5, 0x4b, 0xb3, 0x85, 0x38, 0xeb,
     0xb4, 0xbd, 0x30, 0xeb, 0xf2, 0x80, 0xd8, 0x00,
     0x8d, 0x07, 0xa4, 0xca, 0xf5, 0x42, 0x71, 0xf9,
     0x93, 0x52, 0x7d, 0x46, 0xff, 0x3f, 0xf4, 0x6f,
     0xd1, 0x19, 0x0a, 0x3f, 0x1f, 0xaa, 0x4f, 0x74,
    },
  },
  {
    {0x00, 0x00, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xc0, 0x00, 0x07, 0xff, 0xff, 0xe0, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0xff,
     0xff, 0xff, 0xff, 0xff, },
    {0x00, 0x0e, 0xca, 0x93, 0x42, 0x47, 0x42, 0x5c,
     0xfd, 0x94, 0x9b, 0x79, 0x5c, 0xb5, 0xce, 0x1e,
     0xff, 0x40, 0x15, 0x50, 0x38, 0x6e, 0x28, 0xd1,
     0xa4, 0xc5, 0xa8, 0xeb, 0xd4, 0xc0, 0x10, 0x40,
     0xdb, 0xa1, 0x96, 0x28, 0x93, 0x1b, 0xc8, 0x85,
     0x53, 0x70, 0x31, 0x7c, 0x72, 0x2c, 0xbd, 0x9c,
     0xa6, 0x15, 0x69, 0x85, 0xf1, 0xc2, 0xe9, 0xce,
    },
  },
  {
    {0x7f, 0xff, 0xfc, 0x03, 0xff, 0x80, 0x7f, 0xff,
     0xe0, 0x00, 0x1f, 0xff, 0xff, 0x80, 0x0f, 0xff,
     0x80, 0x00, 0x01, 0xff, 0xff, 0x00, 0x01, 0xff,
     0xff, 0xfe, 0x00, 0x1f, },
    {0xef, 0x35, 0x3b, 0xf5, 0xc7, 0x3c, 0xd5, 0x51,
     0xb9, 0x6d, 0x59, 0x6f, 0xbc, 0x9a, 0x67, 0xf1,
     0x6d, 0x61, 0xdd, 0x9f, 0xe5, 0x6a, 0xf1, 0x9d,
     0xe1, 0xfb, 0xa9, 0xcd, 0x21, 0x77, 0x1b, 0x9c,
     0xdc, 0xe3, 0xe8, 0x43, 0x0c, 0x09, 0xb3, 0x83,
     0x8b, 0xe7, 0x0b, 0x48, 0xc2, 0x1e, 0x15, 0xbc,
     0x09, 0xee, 0x1f, 0x2d, 0x79, 0x45, 0xb9, 0x1f,
    },
  },
  {
    {0x00, 0x00, 0x00, 0x07, 0xff, 0xc0, 0x7f, 0xff,
     0xff, 0xff, 0x01, 0xff, 0xfe, 0x03, 0xff, 0xfe,
     0x40, 0x00, 0x38, 0x00, 0x07, 0xe0, 0x00, 0x3f,
     0xfe, 0x00, 0x00, 0x00, },
    {0x40, 0x36, 0x05, 0x2a, 0x30, 0x91, 0xeb, 0x48,
     0x10, 0x46, 0xad, 0x32, 0x89, 0xc9, 0x5d, 0x3a,
     0xc9, 0x05, 0xca, 0x00, 0x23, 0xde, 0x2c, 0x03,
     0xec, 0xd4, 0x51, 0xcf, 0xd7, 0x68, 0x16, 0x5a,
     0x38, 0xa2, 0xb9, 0x6f, 0x81, 0x25, 0x86, 0xa9,
     0xd5, 0x9d, 0x41, 0x36, 0x03, 0x5d, 0x9c, 0x85,
     0x3a, 0x5b, 0xf2, 0xe1, 0xc8, 0x6a, 0x49, 0x93,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x29, },
    {0xfc, 0xc7, 0xf2, 0xb4, 0x5d, 0xf1, 0xcd, 0x5a,
     0x3c, 0x0c, 0x07, 0x31, 0xca, 0x47, 0xa8, 0xaf,
     0x75, 0xcf, 0xb0, 0x34, 0x7e, 0x83, 0x54, 0xee,
     0xfe, 0x78, 0x24, 0x55, 0xf2, 0xa2, 0x8e, 0xef,
     0xd8, 0xb3, 0x45, 0x83, 0x21, 0x16, 0xf1, 0xe5,
     0x74, 0xf2, 0xc6, 0xb2, 0xc8, 0x95, 0xaa, 0x8c,
     0x24, 0x94, 0x1f, 0x40, 0xd8, 0xb8, 0x0a, 0xd1,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x2a, },
    {0xa1, 0xe8, 0x1c, 0x04, 0xf3, 0x0c, 0xe2, 0x01,
     0xc7, 0xc9, 0xac, 0xe7, 0x85, 0xed, 0x44, 0xcc,
     0x33, 0xb4, 0x55, 0xa0, 0x22, 0xf2, 0xac, 0xdb,
     0xc6, 0xca, 0xe8, 0x3c, 0x23, 0x0e, 0x09, 0x3c,
     0x24, 0xf6, 0x38, 0xf5, 0x33, 0xda, 0xc6, 0xe2,
     0xb6, 0xd0, 0x1d, 0xa3, 0xb5, 0xe7, 0xf4, 0x54,
     0x29, 0x31, 0x5c, 0xa9, 0x3f, 0xb8, 0xe6, 0x34,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x2b, },
    {0xc9, 0xff, 0x61, 0xb0, 0x40, 0x87, 0x4c, 0x05,
     0x68, 0x47, 0x92, 0x16, 0x82, 0x4a, 0x15, 0xea,
     0xb1, 0xa8, 0x38, 0xa7, 0x97, 0xd1, 0x89, 0x74,
     0x62, 0x26, 0xe4, 0xcc, 0x15, 0x67, 0x29, 0xf1,
     0xa0, 0x03, 0x64, 0x70, 0x30, 0x66, 0x60, 0x54,
     0xe2, 0x08, 0x18, 0x0f, 0x8f, 0x7b, 0x0d, 0xf2,
     0x24, 0x9e, 0x44, 0xfb, 0xa5, 0x93, 0x1f, 0xff,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x2c, },
    {0xb8, 0x35, 0x7c, 0x3a, 0x6c, 0xee, 0xf2, 0x88,
     0x31, 0x0e, 0x17, 0xb8, 0xbf, 0xef, 0xf9, 0x20,
     0x08, 0x46, 0xca, 0x8c, 0x19, 0x42, 0x49, 0x7c,
     0x48, 0x44, 0x03, 0xbc, 0x00, 0xeb, 0x61, 0x05,
     0x99, 0xf9, 0x59, 0x42, 0xdf, 0x10, 0x82, 0xe4,
     0xf9, 0x42, 0x6d, 0x08, 0x6f, 0xb9, 0xc6, 0x23,
     0x1a, 0xe8, 0xb2, 0x49, 0x33, 0xaa, 0xb5, 0xdb,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x2d, },
    {0x0b, 0x6e, 0xc4, 0xfe, 0x17, 0x77, 0x38, 0x24,
     0x04, 0xef, 0x67, 0x99, 0x97, 0xba, 0x8d, 0x1c,
     0xc5, 0xcd, 0x8e, 0x85, 0x34, 0x92, 0x59, 0xf5,
     0x90, 0xc4, 0xc6, 0x6d, 0xcc, 0x66, 0x2b, 0x9b,
     0xcb, 0xa6, 0xf9, 0x4e, 0xe4, 0xff, 0x1c, 0x9c,
     0x10, 0xbd, 0x6d, 0xdd, 0x0d, 0x13, 0x8d, 0xf2,
     0xd0, 0x99, 0xa2, 0x82, 0x15, 0x2a, 0x4b, 0x7f,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x2e, },
    {0xba, 0xa4, 0xd8, 0x63, 0x55, 0x11, 0xa7, 0xd2,
     0x88, 0xae, 0xbe, 0xed, 0xd1, 0x2c, 0xe5, 0x29,
     0xff, 0x10, 0x2c, 0x91, 0xf9, 0x7f, 0x86, 0x7e,
     0x21, 0x91, 0x6b, 0xf9, 0x68, 0x65, 0xa0, 0xb8,
     0xa6, 0x07, 0xf0, 0xb0, 0x4b, 0x13, 0xd1, 0xcb,
     0x0a, 0xa9, 0x92, 0xa5, 0xa9, 0x7f, 0x5e, 0xe8,
     0xca, 0x18, 0x49, 0xef, 0xb9, 0xed, 0x86, 0x78,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x2f, },
    {0xa5, 0x36, 0x40, 0xc8, 0x3d, 0xc2, 0x08, 0x60,
     0x3d, 0xed, 0x83, 0xe4, 0xec, 0xf7, 0x58, 0xf2,
     0x4c, 0x35, 0x7d, 0x7c, 0xf4, 0x80, 0x88, 0xb2,
     0xce, 0x01, 0xe9, 0xfa, 0x2a, 0x7e, 0xb3, 0x28,
     0xdb, 0xe6, 0x63, 0xb5, 0xa4, 0x68, 0xb5, 0xbc,
     0x97, 0xa0, 0x40, 0xa3, 0x74, 0x53, 0x96, 0xba,
     0x63, 0x6b, 0x96, 0x43, 0x70, 0xdc, 0x33, 0x52,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x30, },
    {0x34, 0xe8, 0xe1, 0x7a, 0x43, 0x0e, 0x43, 0x28,
     0x97, 0x93, 0xc3, 0x83, 0xfa, 0xc9, 0x77, 0x42,
     0x47, 0xb4, 0x0e, 0x9e, 0xbd, 0x33, 0x66, 0x98,
     0x1f, 0xcf, 0xae, 0xca, 0xda, 0xd7, 0xe6, 0x08,
     0xe3, 0x80, 0x48, 0x04, 0x34, 0xea, 0x64, 0x1c,
     0xc8, 0x2c, 0x82, 0xcb, 0xc9, 0x28, 0x01, 0x46,
     0x9c, 0x8d, 0xb0, 0x20, 0x4f, 0x13, 0x48, 0x9a,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x31, },
    {0x6e, 0x31, 0xee, 0x1d, 0xc1, 0x37, 0xf8, 0x1b,
     0x05, 0x67, 0x52, 0xe4, 0xde, 0xab, 0x14, 0x43,
     0xa4, 0x81, 0x03, 0x3e, 0x9b, 0x4c, 0x93, 0xa3,
     0x04, 0x4f, 0x4f, 0x7a, 0xdf, 0x82, 0x22, 0x0f,
     0xc7, 0xa4, 0x02, 0x15, 0x49, 0x16, 0x53, 0x25,
     0x72, 0x5f, 0x94, 0xc3, 0x41, 0x0d, 0xdb, 0x56,
     0xc5, 0x4e, 0x16, 0x1f, 0xc9, 0xef, 0x62, 0xee,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x32, },
    {0xef, 0x53, 0xb6, 0x29, 0x4a, 0xca, 0x43, 0x1f,
     0x0f, 0x3c, 0x22, 0xdc, 0x82, 0xeb, 0x90, 0x50,
     0x32, 0x4f, 0x1d, 0x88, 0xd3, 0x77, 0xe7, 0x16,
     0x44, 0x8e, 0x50, 0x7c, 0xdf, 0x4a, 0xef, 0xff,
     0xbf, 0x6d, 0x16, 0x99, 0xc9, 0x30, 0x48, 0x1c,
     0xd1, 0x02, 0x12, 0x7c, 0x9a, 0x3d, 0x99, 0x20,
     0x48, 0xab, 0x05, 0x92, 0x9b, 0x6e, 0x59, 0x27,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x33, },
    {0xae, 0xa9, 0xe1, 0x7a, 0x30, 0x65, 0x17, 0xeb,
     0x89, 0x15, 0x2a, 0xa7, 0x09, 0x6d, 0x2c, 0x38,
     0x1e, 0xc8, 0x13, 0xc5, 0x1a, 0xa8, 0x80, 0xe7,
     0xbe, 0xe2, 0xc0, 0xfd, 0xc6, 0x44, 0xcf, 0x15,
     0x4c, 0xc8, 0x1f, 0x5a, 0xde, 0x49, 0x34, 0x5e,
     0x54, 0x1b, 0x4d, 0x4b, 0x5c, 0x1a, 0xdb, 0x3e,
     0xb5, 0xc0, 0x1c, 0x14, 0xee, 0x94, 0x9a, 0xa2,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x34, },
    {0x2f, 0xdc, 0xcc, 0xfe, 0xe7, 0x20, 0xa7, 0x7e,
     0xf6, 0xcb, 0x3b, 0xfb, 0xb4, 0x47, 0xf9, 0x38,
     0x31, 0x17, 0xe3, 0xda, 0xa4, 0xa0, 0x7e, 0x36,
     0xed, 0x15, 0xf7, 0x8d, 0xc8, 0xe8, 0xcd, 0x1b,
     0x0b, 0xe4, 0x0b, 0x08, 0x77, 0xcf, 0xca, 0x19,
     0x58, 0x60, 0x31, 0x22, 0xf1, 0xe6, 0x91, 0x4f,
     0x84, 0xb7, 0xe8, 0xe9, 0x68, 0xae, 0x8b, 0x9e,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x35, },
    {0x85, 0x8e, 0x6f, 0x9c, 0xc6, 0xc1, 0x2c, 0x31,
     0xf5, 0xdf, 0x12, 0x4a, 0xa7, 0x77, 0x67, 0xb0,
     0x5c, 0x8b, 0xc0, 0x21, 0xbd, 0x68, 0x3d, 0x2b,
     0x55, 0x57, 0x15, 0x50, 0xfb, 0x92, 0x32, 0xc1,
     0x5a, 0x3b, 0xc7, 0x67, 0x3a, 0x3a, 0x03, 0xb0,
     0x25, 0x38, 0x24, 0xc5, 0x3d, 0x0f, 0xd1, 0x41,
     0x1b, 0x1c, 0xab, 0xe2, 0xe1, 0x87, 0xfb, 0x87,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x36, },
    {0xdb, 0x2f, 0x6b, 0xe6, 0x30, 0xe2, 0x46, 0xa5,
     0xcf, 0x7d, 0x99, 0xb8, 0x51, 0x94, 0xb1, 0x23,
     0xd4, 0x87, 0xe2, 0xd4, 0x66, 0xb9, 0x4b, 0x24,
     0xa0, 0x3c, 0x3e, 0x28, 0xf0, 0xc5, 0xcf, 0xf7,
     0xab, 0x68, 0x0d, 0x09, 0xee, 0x11, 0xda, 0xe8,
     0x4e, 0x9c, 0x10, 0x72, 0xac, 0x48, 0xea, 0x2e,
     0x74, 0x4b, 0x1b, 0x7f, 0x72, 0xfd, 0x46, 0x9e,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x37, },
    {0x1f, 0x24, 0x83, 0xf8, 0x25, 0x72, 0x25, 0x1f,
     0xca, 0x97, 0x5f, 0xea, 0x40, 0xdb, 0x82, 0x1d,
     0xf8, 0xad, 0x82, 0xa3, 0xc0, 0x02, 0xee, 0x6c,
     0x57, 0x11, 0x24, 0x08, 0x76, 0x05, 0x0f, 0x33,
     0x48, 0xaf, 0x26, 0x64, 0xaa, 0xc3, 0xa8, 0xb0,
     0x52, 0x81, 0x30, 0x4e, 0xbc, 0x7a, 0x79, 0x14,
     0xc6, 0xad, 0x50, 0xa4, 0xb4, 0xea, 0xc3, 0x83,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x38, },
    {0x31, 0xc4, 0x9a, 0xe7, 0x5b, 0xce, 0x78, 0x07,
     0xcd, 0xff, 0x22, 0x05, 0x5d, 0x94, 0xee, 0x90,
     0x21, 0xfe, 0xdb, 0xb5, 0xab, 0x51, 0xc5, 0x75,
     0x26, 0xf0, 0x11, 0xaa, 0xd8, 0x17, 0x40, 0x0e,
     0x8b, 0xa9, 0xca, 0x13, 0xa4, 0x5f, 0x36, 0x0e,
     0x3d, 0x12, 0x1e, 0xaa, 0xeb, 0x39, 0xaf, 0x82,
     0xd6, 0x00, 0x1c, 0x81, 0x86, 0xf5, 0xf8, 0x66,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x39, },
    {0xae, 0x99, 0xfe, 0xeb, 0xb5, 0xd2, 0x69, 0x45,
     0xb5, 0x48, 0x92, 0x09, 0x2a, 0x8a, 0xee, 0x02,
     0x91, 0x29, 0x30, 0xfa, 0x41, 0xcd, 0x11, 0x4e,
     0x40, 0x44, 0x73, 0x01, 0xfb, 0x7d, 0xa7, 0xf5,
     0xf1, 0x3a, 0x43, 0xb8, 0x17, 0x74, 0x37, 0x3c,
     0x87, 0x9c, 0xd3, 0x2d, 0x69, 0x34, 0xc0, 0x5f,
     0xa7, 0x58, 0xee, 0xb1, 0x4f, 0xcf, 0xab, 0x38,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x3a, },
    {0xdf, 0x1b, 0x1d, 0x66, 0xa5, 0x51, 0xd0, 0xd3,
     0x1e, 0xff, 0x82, 0x25, 0x58, 0xb9, 0xd2, 0xcc,
     0x75, 0xc2, 0x18, 0x02, 0x79, 0xfe, 0x0d, 0x08,
     0xfd, 0x89, 0x6d, 0x04, 0x5c, 0x08, 0x0f, 0xc3,
     0x52, 0x2f, 0x41, 0xbb, 0xb3, 0xf5, 0x5a, 0x97,
     0xcf, 0xec, 0xf2, 0x1f, 0x88, 0x2c, 0xe8, 0xcb,
     0xb1, 0xe5, 0x0c, 0xa6, 0xe6, 0x7e, 0x56, 0xdc,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x3b, },
    {0x70, 0x6a, 0x46, 0xdc, 0x76, 0xdc, 0xb7, 0x67,
     0x98, 0xe6, 0x0e, 0x6d, 0x89, 0x47, 0x47, 0x88,
     0xd1, 0x6d, 0xc1, 0x80, 0x32, 0xd2, 0x68, 0xfd,
     0x1a, 0x70, 0x4f, 0xa6, 0xe3, 0xd4, 0x89, 0x58,
     0x43, 0xda, 0x18, 0x8f, 0xd5, 0x8f, 0xb0, 0x56,
     0x79, 0x76, 0xd7, 0xb5, 0x03, 0x59, 0xd6, 0xb7,
     0x85, 0x30, 0xc8, 0xf6, 0x2d, 0x1b, 0x17, 0x46,
    },
  },
  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x16, 0xa2,
     0xe0, 0xb8, 0xf0, 0x3e, 0x13, 0xdd, 0x29, 0x45,
     0x5c, 0x5c, 0x2a, 0x3c, },
    {0xb7, 0x0e, 0x0c, 0xbd, 0x6b, 0xb4, 0xbf, 0x7f,
     0x32, 0x13, 0x90, 0xb9, 0x4a, 0x03, 0xc1, 0xd3,
     0x56, 0xc2, 0x11, 0x22, 0x34, 0x32, 0x80, 0xd6,
     0x11, 0x5c, 0x1d, 0x21, 0x42, 0xc8, 0x9c, 0x77,
     0x4a, 0x08, 0xdc, 0x04, 0xb3, 0xdd, 0x20, 0x19,
     0x32, 0xbc, 0x8a, 0x5e, 0xa5, 0xf8, 0xb8, 0x9b,
     0xbb, 0x2a, 0x7e, 0x66, 0x7a, 0xff, 0x81, 0xcd,
    },
  },
};

TEST(P224, ExternalToInternalAndBack) {
  Point point;

  EXPECT_TRUE(point.SetFromString(base::StringPiece(
      reinterpret_cast<const char *>(kBasePointExternal),
      sizeof(kBasePointExternal))));

  const std::string external = point.ToString();

  ASSERT_EQ(external.size(), 56u);
  EXPECT_TRUE(memcmp(external.data(), kBasePointExternal,
                     sizeof(kBasePointExternal)) == 0);
}

TEST(P224, ScalarBaseMult) {
  Point point;

  for (size_t i = 0; i < arraysize(kNISTTestVectors); i++) {
    p224::ScalarBaseMult(kNISTTestVectors[i].scalar, &point);
    const std::string external = point.ToString();
    ASSERT_EQ(external.size(), 56u);
    EXPECT_TRUE(memcmp(external.data(), kNISTTestVectors[i].affine,
                       external.size()) == 0);
  }
}

TEST(P224, Addition) {
  Point a, b, minus_b, sum, a_again;

  ASSERT_TRUE(a.SetFromString(base::StringPiece(
      reinterpret_cast<const char *>(kNISTTestVectors[10].affine), 56)));
  ASSERT_TRUE(b.SetFromString(base::StringPiece(
      reinterpret_cast<const char *>(kNISTTestVectors[11].affine), 56)));

  p224::Negate(b, &minus_b);
  p224::Add(a, b, &sum);
  EXPECT_TRUE(memcmp(&sum, &a, sizeof(sum) != 0));
  p224::Add(minus_b, sum, &a_again);
  EXPECT_TRUE(a_again.ToString() == a.ToString());
}
