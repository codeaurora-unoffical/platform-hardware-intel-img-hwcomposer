/*
 * Copyright © 2012 Intel Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Jackie Li <yaodong.li@intel.com>
 *
 */
#include <HwcTrace.h>
#include <Drm.h>
#include <Hwcomposer.h>
#include <penwell/PnwGrallocBufferMapper.h>

namespace android {
namespace intel {

PnwGrallocBufferMapper::PnwGrallocBufferMapper(DataBuffer& buffer)
    : GrallocBufferMapperBase(buffer)
{
    CTRACE();
}

PnwGrallocBufferMapper::~PnwGrallocBufferMapper()
{
    CTRACE();
}

bool PnwGrallocBufferMapper::map()
{
    // TODO: implement map
    return false;
}

bool PnwGrallocBufferMapper::unmap()
{
    //TODO: implement unmap
    return false;
}

} // namespace intel
} // namespace android