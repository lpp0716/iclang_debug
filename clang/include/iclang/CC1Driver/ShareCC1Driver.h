//===--- ShareCC1Driver.h - IClang cc1 driver for shared compilation ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===/
//
// IClang cc1 driver for shared compilation.
//
//===----------------------------------------------------------------------===/

#ifndef ICLANG_SHARECC1DRIVER_H
#define ICLANG_SHARECC1DRIVER_H

namespace iclang {

class ShareMasterCC1Driver {
public:
  static void run();
};

class ShareClientCC1Driver {
public:
  static void run();
};

class ShareCheckCC1Driver {
public:
  static void run();
};

} // namespace iclang

#endif //ICLANG_SHARECC1DRIVER_H
