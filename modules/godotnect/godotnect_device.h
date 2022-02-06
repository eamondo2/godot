#ifndef GODOTNECT_DEVICE_H
#define GODOTNECT_DEVICE_H

#include "libfreenect.h"
#include "libfreenect.hpp"

#include "core/object/class_db.h"

class GodotNectTiltState {
  friend class GodotNectDevice;
  GodotNectTiltState(freenect_raw_tilt_state *_state)
      : m_code(_state->tilt_status), m_state(_state) {}

public:
  void getAccelerometers(double *x, double *y, double *z) {
    freenect_get_mks_accel(m_state, x, y, z);
  }
  double getTiltDegs() { return freenect_get_tilt_degs(m_state); }

public:
  freenect_tilt_status_code m_code;

private:
  freenect_raw_tilt_state *m_state;
};

//=============================================================================
// Device implementation, keeping it here and out of the way for convenience
class GodotNectDevice {
public:
  freenect_context *f_ctx; // Stash the context here for use.

  // Testing adding the extra params so that we can pass specific depth/video
  // format options at instantiation time.
  GodotNectDevice(freenect_context *_ctx, std::string _index,
                  freenect_depth_format depth_fmt,
                  freenect_video_format video_fmt,
                  freenect_resolution resolution_fmt);

  virtual ~GodotNectDevice();

  void startVideo() {
    if (freenect_start_video(m_dev) < 0) {
      throw std::runtime_error("Cannot start RGB callback");
    }
  }
  void stopVideo() {
    if (freenect_stop_video(m_dev) < 0) {
      throw std::runtime_error("Cannot stop RGB callback");
    }
  }
  void startDepth() {
    if (freenect_start_depth(m_dev) < 0) {
      throw std::runtime_error("Cannot start depth callback");
    }
  }
  void stopDepth() {
    if (freenect_stop_depth(m_dev) < 0) {
      throw std::runtime_error("Cannot stop depth callback");
    }
  }
  void setTiltDegrees(double _angle) {
    if (freenect_set_tilt_degs(m_dev, _angle) < 0) {
      throw std::runtime_error("Cannot set angle in degrees");
    }
  }
  void setLed(freenect_led_options _option) {
    if (freenect_set_led(m_dev, _option) < 0) {
      throw std::runtime_error("Cannot set led");
    }
  }
  void updateState() {
    if (freenect_update_tilt_state(m_dev) < 0) {
      throw std::runtime_error("Cannot update device state");
    }
  }
  GodotNectTiltState getState() const {
    return GodotNectTiltState(freenect_get_tilt_state(m_dev));
  }
  void setVideoFormat(
      freenect_video_format requested_format,
      freenect_resolution requested_resolution = FREENECT_RESOLUTION_MEDIUM) {
    if (requested_format != m_video_format ||
        requested_resolution != m_video_resolution) {
      const bool wasRunning = (freenect_stop_video(m_dev) >= 0);
      const freenect_frame_mode mode =
          freenect_find_video_mode(requested_resolution, requested_format);
      if (!mode.is_valid) {
        throw std::runtime_error("Cannot set video format: invalid mode");
      }
      if (freenect_set_video_mode(m_dev, mode) < 0) {
        throw std::runtime_error("Cannot set video format");
      }

      m_video_format = requested_format;
      m_video_resolution = requested_resolution;
      m_rgb_buffer.reset(new uint8_t[mode.bytes]);
      freenect_set_video_buffer(m_dev, m_rgb_buffer.get());

      if (wasRunning) {
        freenect_start_video(m_dev);
      }
    }
  }
  freenect_video_format getVideoFormat() { return m_video_format; }
  freenect_resolution getVideoResolution() { return m_video_resolution; }
  void setDepthFormat(
      freenect_depth_format requested_format,
      freenect_resolution requested_resolution = FREENECT_RESOLUTION_MEDIUM) {
    if (requested_format != m_depth_format ||
        requested_resolution != m_depth_resolution) {
      bool wasRunning = (freenect_stop_depth(m_dev) >= 0);
      freenect_frame_mode mode =
          freenect_find_depth_mode(requested_resolution, requested_format);
      if (!mode.is_valid) {
        throw std::runtime_error("Cannot set depth format: invalid mode");
      }
      if (freenect_set_depth_mode(m_dev, mode) < 0) {
        throw std::runtime_error("Cannot set depth format");
      }
      if (wasRunning) {
        freenect_start_depth(m_dev);
      }
      m_depth_format = requested_format;
      m_depth_resolution = requested_resolution;
    }
  }
  freenect_depth_format getDepthFormat() { return m_depth_format; }
  freenect_resolution getDepthResolution() { return m_depth_resolution; }
  int setFlag(freenect_flag flag, bool value) {
    return freenect_set_flag(m_dev, flag, value ? FREENECT_ON : FREENECT_OFF);
  }
  const freenect_device *getDevice() { return m_dev; }
  // Do not call directly even in child
  virtual void VideoCallback(void *video, uint32_t timestamp);
  // Do not call directly even in child
  virtual void DepthCallback(void *depth, uint32_t timestamp);

protected:
  int getVideoBufferSize() {
    switch (m_video_format) {
    case FREENECT_VIDEO_RGB:
    case FREENECT_VIDEO_BAYER:
    case FREENECT_VIDEO_IR_8BIT:
    case FREENECT_VIDEO_IR_10BIT:
    case FREENECT_VIDEO_IR_10BIT_PACKED:
    case FREENECT_VIDEO_YUV_RGB:
    case FREENECT_VIDEO_YUV_RAW:
      return freenect_find_video_mode(m_video_resolution, m_video_format).bytes;
    default:
      return 0;
    }
  }
  int getDepthBufferSize() {
    return freenect_get_current_depth_mode(m_dev).bytes;
  }

private:
  Vector<uint16_t> depth_data;
  Vector<uint8_t> video_data;

  freenect_device *m_dev;

  freenect_video_format m_video_format;
  freenect_depth_format m_depth_format;
  freenect_resolution m_video_resolution;
  freenect_resolution m_depth_resolution;

  std::unique_ptr<uint8_t[]> m_rgb_buffer;

  static void freenect_depth_callback(freenect_device *dev, void *depth,
                                      uint32_t timestamp) {
    GodotNectDevice *device =
        static_cast<GodotNectDevice *>(freenect_get_user(dev));
    device->DepthCallback(depth, timestamp);
  }
  static void freenect_video_callback(freenect_device *dev, void *video,
                                      uint32_t timestamp) {
    GodotNectDevice *device =
        static_cast<GodotNectDevice *>(freenect_get_user(dev));
    device->VideoCallback(video, timestamp);
  }
};

#endif