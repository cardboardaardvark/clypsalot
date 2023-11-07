#pragma once

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>
#include <QWidget>

#include <clypsalot/object.hxx>

#include "forward.hxx"

using ObjectId = size_t;

static const QString objectMimeFormat("application/x-clypsalot-object");
static const QString outputPortMimeFormat("application/x-clypsalot-output-port");

class PortConnectionPoint : public QWidget
{
    protected:
    virtual void paintEvent(QPaintEvent* event) override;

    public:
    explicit PortConnectionPoint(QWidget* parent = nullptr);
};

class Port : public QFrame
{
    Q_OBJECT

    protected:
    QHBoxLayout* mainLayout = nullptr;
    QLabel* nameLabel = nullptr;
    PortConnectionPoint* connectPoint = nullptr;
    QVBoxLayout* connectPointLayout = nullptr;

    explicit Port(const QString& name, QWidget* parent = nullptr);

    public:
    QPoint globalConnectPointPosition();
    Object* parentObject();
    QString name();
};

class InputPort : public Port
{
    protected:
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;

    public:
    InputPort(const QString& name, QWidget* parent = nullptr);
};

class OutputPort : public Port
{
    QPoint dragStart;

    protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

    public:
    OutputPort(const QString& name, QWidget* parent = nullptr);
};

class Object : public QWidget
{
    Q_OBJECT

    QHBoxLayout* mainLayout = nullptr;
    QVBoxLayout* inputLayout = nullptr;
    QVBoxLayout* outputLayout = nullptr;
    QFrame* infoFrame = nullptr;
    QVBoxLayout* infoLayout = nullptr;
    QLabel* kindLabel = nullptr;
    QLabel* stateLabel = nullptr;
    std::vector<std::shared_ptr<Clypsalot::Subscription>> subscriptions;
    QPoint dragStartPoint;
    bool dragInProgress = false;

    void addPorts();
    void addOutput(const QString& name);
    void addInput(const QString& name);
    void handleEvent(const Clypsalot::ObjectStateChangedEvent& event);
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    void dragStart();

    protected Q_SLOTS:
    void dragDestroyed();
    void updateState(const Clypsalot::ObjectState newState);

    public:
    const ObjectId id;
    const Clypsalot::SharedObject object;

    explicit Object(const size_t id, const Clypsalot::SharedObject& object, QWidget *parent = nullptr);
    QPoint& dragStartPosition();
    void dragEnd();

    Q_SIGNALS:
    void stateChanged(const Clypsalot::ObjectState newState);
};

class ObjectMimeData : public QMimeData
{
    Q_OBJECT

    public:
    Object* const object;

    ObjectMimeData(Object* const object);
};

class OutputPortMimeData : public QMimeData
{
    Q_OBJECT

    public:
    OutputPort* const port;

    OutputPortMimeData(OutputPort* const port);
};
