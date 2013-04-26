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
#include <fcntl.h>
#include <errno.h>
#include <HwcTrace.h>
#include <IDisplayDevice.h>
#include <DrmConfig.h>
#include <Drm.h>

namespace android {
namespace intel {

Drm::Drm()
{
    const char *path = DrmConfig::getDrmPath();
    int fd = open(path, O_RDWR, 0);
    if (fd < 0) {
        ETRACE("drmOpen failed, error: %s", strerror(errno));
    }

    mDrmFd = fd;
    memset(&mOutputs, 0, sizeof(mOutputs));

    DTRACE("mDrmFd = %d", fd);
}

bool Drm::detect()
{
    Mutex::Autolock _l(mLock);

    if (mDrmFd < 0) {
        ETRACE("invalid Fd");
        return false;
    }

    // try to get drm resources
    drmModeResPtr resources = drmModeGetResources(mDrmFd);
    if (!resources) {
        ETRACE("fail to get drm resources, error: %s", strerror(errno));
        return false;
    }

    drmModeConnectorPtr connector = NULL;
    drmModeEncoderPtr encoder = NULL;
    drmModeCrtcPtr crtc = NULL;
    drmModeFBPtr fbInfo = NULL;
    struct Output *output = NULL;

    const uint32_t primaryConnector =
        DrmConfig::getDrmConnector(IDisplayDevice::DEVICE_PRIMARY);
    const uint32_t externalConnector =
        DrmConfig::getDrmConnector(IDisplayDevice::DEVICE_EXTERNAL);

    for (int i = 0; i < resources->count_connectors; i++) {
        connector = drmModeGetConnector(mDrmFd, resources->connectors[i]);
        if (!connector) {
            ETRACE("failed to get drm connector");
            continue;
        }

        int outputIndex = -1;
        if (connector->connector_type == primaryConnector) {
            VTRACE("got primary connector");
            outputIndex = OUTPUT_PRIMARY;
        } else if (connector->connector_type == externalConnector) {
            VTRACE("got external connector");
            outputIndex = OUTPUT_EXTERNAL;
        }

        if (outputIndex < 0)
            continue;

        // update output, free the old objects first
        output = &mOutputs[outputIndex];

        output->connected =  0;

        if (output->connector) {
            drmModeFreeConnector(output->connector);
            output->connector = 0;
        }
        if (output->encoder) {
            drmModeFreeEncoder(output->encoder);
            output->encoder = 0;
        }

        if (output->crtc) {
            drmModeFreeCrtc(output->crtc);
            output->crtc = 0;
        }

        if (output->fb) {
            drmModeFreeFB(output->fb);
            output->fb = 0;
        }

        output->connector = connector;

        // get current encoder
        encoder = drmModeGetEncoder(mDrmFd, connector->encoder_id);
        if (!encoder) {
            ETRACE("failed to get drm encoder");
            continue;
        }

        output->encoder = encoder;

        // get crtc
        crtc = drmModeGetCrtc(mDrmFd, encoder->crtc_id);
        if (!crtc) {
            ETRACE("failed to get drm crtc");
            continue;
        }

        output->crtc = crtc;

        // get fb info
        fbInfo = drmModeGetFB(mDrmFd, crtc->buffer_id);
        if (!fbInfo) {
            ETRACE("failed to get fb info");
            continue;
        }

        output->fb = fbInfo;

        output->connected = (output->connector &&
                             output->connector->connection == DRM_MODE_CONNECTED &&
                             output->encoder &&
                             output->crtc &&
                             output->fb) ? 1 : 0;
    }

    drmModeFreeResources(resources);

    return true;
}

bool Drm::writeReadIoctl(unsigned long cmd, void *data,
                           unsigned long size)
{
    int err;

    Mutex::Autolock _l(mLock);

    if (mDrmFd <= 0) {
        ETRACE("drm is not initialized");
        return false;
    }

    if (!data || !size) {
        ETRACE("invalid parameters");
        return false;
    }

    err = drmCommandWriteRead(mDrmFd, cmd, data, size);
    if (err) {
        ETRACE("failed to call %ld ioctl with failure %d", cmd, err);
        return false;
    }

    return true;
}

bool Drm::writeIoctl(unsigned long cmd, void *data,
                       unsigned long size)
{
    int err;

    Mutex::Autolock _l(mLock);

    if (mDrmFd <= 0) {
        ETRACE("drm is not initialized");
        return false;
    }

    if (!data || !size) {
        ETRACE("invalid parameters");
        return false;
    }

    err = drmCommandWrite(mDrmFd, cmd, data, size);
    if (err) {
        ETRACE("failed to call %ld ioctl with failure %d", cmd, err);
        return false;
    }

    return true;
}

int Drm::getDrmFd() const
{
    return mDrmFd;
}

struct Output* Drm::getOutput(int device)
{
    Mutex::Autolock _l(mLock);

    int output = getOutputIndex(device);
    if (output < 0 ) {
        return 0;
    }

    return &mOutputs[output];
}

bool Drm::outputConnected(int device)
{
    Mutex::Autolock _l(mLock);

    int output = getOutputIndex(device);
    if (output < 0 ) {
        return false;
    }

    return mOutputs[output].connected ? true : false;
}

bool Drm::setDpmsMode(int device, int mode)
{
    Mutex::Autolock _l(mLock);

    int output = getOutputIndex(device);
    if (output < 0 ) {
        return false;
    }

    if (mode != IDisplayDevice::DEVICE_DISPLAY_OFF &&
        mode != IDisplayDevice::DEVICE_DISPLAY_ON) {
        ETRACE("invalid mode %d", mode);
        return false;
    }

    Output *out = &mOutputs[output];
    if (out->connector == NULL) {
        ETRACE("invalid connector");
        return false;
    }

    drmModePropertyPtr props;
    for (int i = 0; i < out->connector->count_props; i++) {
        props = drmModeGetProperty(mDrmFd, out->connector->props[i]);
        if (!props) {
            continue;
        }

        if (strcmp(props->name, "DPMS") == 0) {
            int ret = drmModeConnectorSetProperty(
                mDrmFd,
                out->connector->connector_id,
                props->prop_id,
                (mode == IDisplayDevice::DEVICE_DISPLAY_ON) ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF);
            drmModeFreeProperty(props);
            if (ret != 0) {
                ETRACE("unable to set DPMS %d", mode);
                return false;
            } else {
                return true;
            }
        }
        drmModeFreeProperty(props);
    }
    return false;
}

int Drm::getOutputIndex(int device)
{
    switch (device) {
    case IDisplayDevice::DEVICE_PRIMARY:
        return OUTPUT_PRIMARY;
    case IDisplayDevice::DEVICE_EXTERNAL:
        return OUTPUT_EXTERNAL;
    default:
        ETRACE("invalid display device");
        break;
    }

    return -1;
}


} // namespace intel
} // namespace android
