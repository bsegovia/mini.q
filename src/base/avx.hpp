/*-------------------------------------------------------------------------
 - mini.q - a minimalistic multiplayer FPS
 - avx.hpp -> exposes AVX classes and functions
 -------------------------------------------------------------------------*/
// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //
#pragma once
#include "sse.hpp"

namespace q {
  struct avxb;
  struct avxi;
  struct avxf;
} /* namespace q */

#include "avxb.hpp"
#if defined (__AVX2__)
#include "avxi.hpp"
#else
#include "avxi_emu.hpp"
#endif
#include "avxf.hpp"

