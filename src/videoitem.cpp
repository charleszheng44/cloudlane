#include "videoitem.h"
#include "backend.h"
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <mpv/render_gl.h>

// Qt owns this object on its render thread. Only mpv_render_* calls belong
// here.
class VideoRenderer final : public QQuickFramebufferObject::Renderer {
public:
  explicit VideoRenderer(const VideoItem *item)
      : target(const_cast<VideoItem *>(item)) {
    auto backend = item->backend();
    if (!backend || !backend->playerHandle())
      return;
    mpv_opengl_init_params gl{
        [](void *, const char *name) -> void * {
          return reinterpret_cast<void *>(
              QOpenGLContext::currentContext()->getProcAddress(name));
        },
        nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE,
         const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl},
        {MPV_RENDER_PARAM_INVALID, nullptr}};
    const int result =
        mpv_render_context_create(&context, backend->playerHandle(), params);
    if (result < 0) {
      QMetaObject::invokeMethod(
          backend,
          [backend, result] {
            emit backend->message(QStringLiteral("视频渲染初始化失败：") +
                                  mpv_error_string(result));
          },
          Qt::QueuedConnection);
      return;
    }
    mpv_render_context_set_update_callback(
        context,
        [](void *data) {
          auto self = static_cast<VideoRenderer *>(data);
          // The item outlives its renderer. A queued call is removed if Qt
          // destroys it.
          QMetaObject::invokeMethod(self->target, "update",
                                    Qt::QueuedConnection);
        },
        this);
  }
  ~VideoRenderer() override {
    if (context) {
      mpv_render_context_set_update_callback(context, nullptr, nullptr);
      mpv_render_context_free(context);
    }
  }
  QOpenGLFramebufferObject *
  createFramebufferObject(const QSize &size) override {
    return new QOpenGLFramebufferObject(
        size, QOpenGLFramebufferObject::CombinedDepthStencil);
  }
  void render() override {
    QQuickOpenGLUtils::resetOpenGLState();
    if (context) {
      mpv_render_context_update(context);
      auto fbo = framebufferObject();
      mpv_opengl_fbo framebuffer{static_cast<int>(fbo->handle()), fbo->width(),
                                 fbo->height(), 0};
      int flip = 1;
      mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &framebuffer},
                                   {MPV_RENDER_PARAM_FLIP_Y, &flip},
                                   {MPV_RENDER_PARAM_INVALID, nullptr}};
      mpv_render_context_render(context, params);
    } else {
      auto gl = QOpenGLContext::currentContext()->functions();
      gl->glClearColor(0, 0, 0, 1);
      gl->glClear(GL_COLOR_BUFFER_BIT);
    }
    QQuickOpenGLUtils::resetOpenGLState();
  }

private:
  mpv_render_context *context = nullptr;
  VideoItem *target;
};
VideoItem::VideoItem(QQuickItem *parent) : QQuickFramebufferObject(parent) {}
void VideoItem::setBackend(Backend *value) {
  if (owner == value)
    return;
  owner = value;
  emit backendChanged();
  update();
}
QQuickFramebufferObject::Renderer *VideoItem::createRenderer() const {
  return new VideoRenderer(this);
}
