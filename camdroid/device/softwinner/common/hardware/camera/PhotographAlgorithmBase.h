#ifndef __PHOTOGRAPH_ALGORITHM_BASE_H__
#define __PHOTOGRAPH_ALGORITHM_BASE_H__

#include <type_camera.h>

namespace android {

class PhotographAlgorithmBase {
public:
    PhotographAlgorithmBase() {}
    virtual ~PhotographAlgorithmBase() {}

    virtual status_t    initialize(int width, int height, int thumbWidth, int thumbHeight, int fps) = 0;
    virtual status_t    destroy() = 0;
    virtual status_t    camDataReady(V4L2BUF_t *v4l2_buf) = 0;
    virtual status_t    enable(bool enable) = 0;
    virtual bool        thread() = 0;
private:
    virtual status_t    getCurrentFrame(V4L2BUF_t *v4l2_buf) = 0;
};

};

#endif /* __PHOTOGRAPH_ALGORITHM_BASE_H__ */
