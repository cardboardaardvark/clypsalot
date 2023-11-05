#include <QApplication>
#include <QDrag>
#include <QMouseEvent>

#include "logging.hxx"
#include "main.hxx"
#include "object.hxx"

using namespace std::placeholders;

PortConnectionPoint::PortConnectionPoint(QWidget* parent) :
    QWidget(parent)
{
    setMinimumSize(12, 12);
}

void PortConnectionPoint::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.setBrush(Qt::SolidPattern);
    painter.drawEllipse(1, 1, 10, 10);
}

Port::Port(const QString& name, QWidget* parent) :
    QFrame(parent)
{
    mainLayout = new QHBoxLayout();
    setLayout(mainLayout);

    nameLabel = new QLabel(name);

    connectPoint = new PortConnectionPoint(this);
    connectPoint->show();

    connectPointLayout = new QVBoxLayout();
    connectPointLayout->addItem(makeSpacer());
    connectPointLayout->addWidget(connectPoint);
    connectPointLayout->addItem(makeSpacer());
}

QPoint Port::globalConnectPointPosition()
{
    auto position = connectPoint->pos();
    auto size = connectPoint->size();

    position.setX(position.x() + size.width() / 2);
    position.setY(position.y() + size.height() / 2);
    return mapToGlobal(position);
}

Object* Port::parentObject()
{
    return qobject_cast<Object *>(parent());
}

QString Port::name()
{
    return nameLabel->text();
}

InputPort::InputPort(const QString& name, QWidget* parent) :
    Port(name, parent)
{
    setAcceptDrops(true);

    mainLayout->addLayout(connectPointLayout);
    mainLayout->addWidget(nameLabel);
}

void InputPort::dragEnterEvent(QDragEnterEvent* event)
{
    auto mime = event->mimeData();

    if (mime->hasFormat(outputPortMimeFormat))
    {
        event->acceptProposedAction();
    }
}

void InputPort::dropEvent(QDropEvent* event)
{
    if (! event->mimeData()->hasFormat(outputPortMimeFormat)) return;

    auto mime = qobject_cast<const OutputPortMimeData*>(event->mimeData());

    mainWindow()->workArea()->connectPortsCommand(mime->port, this);
}

OutputPort::OutputPort(const QString& name, QWidget* parent) :
    Port(name, parent)
{
    mainLayout->addWidget(nameLabel);
    mainLayout->addLayout(connectPointLayout);
}

void OutputPort::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragStart = event->pos();
    }
}

void OutputPort::mouseMoveEvent(QMouseEvent* event)
{
    if (! event->buttons() & Qt::LeftButton) return;
    if ((event->pos() - dragStart).manhattanLength() < QApplication::startDragDistance()) return;

    auto drag = new QDrag(this);
    auto mime = new OutputPortMimeData(this);

    connect(drag, SIGNAL(destroyed()), mainWindow()->workArea(), SLOT(clearTempConnectionLine()));

    drag->setMimeData(mime);
    drag->exec();
}

Object::Object(const ObjectId id, const Clypsalot::SharedObject& object, QWidget* parent) :
    QWidget(parent),
    id(id),
    object(object)
{
    std::unique_lock lock(*object);

    setStyleSheet("background-color: white;");

    mainLayout = new QHBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    inputLayout = new QVBoxLayout();
    inputLayout->addItem(makeSpacer());
    mainLayout->addLayout(inputLayout);

    infoFrame = new QFrame();
    infoFrame->setFrameStyle(QFrame::Box);
    infoFrame->setLineWidth(2);
    mainLayout->addWidget(infoFrame);

    infoLayout = new QVBoxLayout();
    infoLayout->setSizeConstraint(QLayout::SetFixedSize);
    infoFrame->setLayout(infoLayout);

    kindLabel = new QLabel(QString::fromStdString(object->kind));
    infoLayout->addWidget(kindLabel);

    auto stateLayout = new QHBoxLayout();
    stateLabel = new QLabel();
    stateLayout->addItem(makeSpacer());
    stateLayout->addWidget(stateLabel);
    stateLayout->addItem(makeSpacer());
    infoLayout->addLayout(stateLayout);

    infoLayout->addSpacerItem(makeSpacer());

    outputLayout = new QVBoxLayout();
    outputLayout->addItem(makeSpacer());
    mainLayout->addLayout(outputLayout);

    addPorts();

    subscriptions.push_back(object->subscribe<Clypsalot::ObjectStateChangedEvent>(std::bind(&Object::handleEvent, this, _1)));
    connect(this, SIGNAL(stateChanged(Clypsalot::ObjectState)), this, SLOT(updateState(Clypsalot::ObjectState)));
    updateState(object->state());
}

void Object::addPorts()
{
    assert(object->haveLock());

    for (const auto output : object->outputs())
    {
        addOutput(QString::fromStdString(output->name));
    }

    for (const auto input : object->inputs())
    {
        addInput(QString::fromStdString(input->name));
    }
}

void Object::addOutput(const QString& name)
{
    auto port = new OutputPort(name);

    outputLayout->addWidget(port);
    outputLayout->addItem(makeSpacer());
}

void Object::addInput(const QString& name)
{
    auto port = new InputPort(name);

    inputLayout->addWidget(port);
    inputLayout->addItem(makeSpacer());
}

void Object::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragStartPoint = event->position().toPoint();
        return;
    }

    QWidget::mousePressEvent(event);
}

void Object::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && (event->position().toPoint() - dragStartPoint).manhattanLength() < QApplication::startDragDistance())
    {
        dragStart();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void Object::dragStart()
{
    assert(! dragInProgress);

    dragInProgress = true;

    auto drag = new QDrag(this);
    auto mime = new ObjectMimeData(this);

    connect(drag, SIGNAL(destroyed()), this, SLOT(dragDestroyed()));

    drag->setMimeData(mime);
    drag->exec();
}

void Object::dragDestroyed()
{
    if (dragInProgress)
    {
        // Put the object back if the drag was cancelled
        move(dragStartPoint);
        dragEnd();
    }
}

void Object::dragEnd()
{
    assert(dragInProgress);
    dragInProgress = false;
}

QPoint& Object::dragStartPosition()
{
    return dragStartPoint;
}

void Object::handleEvent(const Clypsalot::ObjectStateChangedEvent& event)
{
    Q_EMIT stateChanged(event.newState);
}

void Object::updateState(const Clypsalot::ObjectState newState)
{
    stateLabel->setText(QString::fromStdString(Clypsalot::asString(newState)));
}

ObjectMimeData::ObjectMimeData(Object* const object) :
    QMimeData(),
    object(object)
{
    setData(objectMimeFormat, "");
}

OutputPortMimeData::OutputPortMimeData(OutputPort* const port) :
    QMimeData(),
    port(port)
{
    setData(outputPortMimeFormat, "");
}
