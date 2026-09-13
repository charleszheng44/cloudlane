#pragma once
#include "backend.h"
#include <QQuickFramebufferObject>
class VideoItem : public QQuickFramebufferObject {
  Q_OBJECT
  Q_PROPERTY(
      Backend *backend READ backend WRITE setBackend NOTIFY backendChanged)
public:
  explicit VideoItem(QQuickItem *parent = nullptr);
  Backend *backend() const { return owner; }
  void setBackend(Backend *value);
  Renderer *createRenderer() const override;
signals:
  void backendChanged();

private:
  Backend *owner = nullptr;
};
