#include <cassert>

#include <QDragEnterEvent>
#include <QMimeData>
#include <QScrollArea>
#include <QScrollBar>

#include "catalog.hxx"
#include "logging.hxx"
#include "main.hxx"
#include "object.hxx"
#include "workarea.hxx"

ObjectId AddObjectCommand::nextObjectId = 1;
const int scrollAreaMargin = 50;

WorkArea::WorkArea(QWidget* parent)
    : QWidget(parent)
{ }

void WorkArea::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.setBrush(Qt::SolidPattern);
    painter.setRenderHints(QPainter::Antialiasing, true);

    if (tempConnectionLine != nullptr) drawPortConnection(painter, (*tempConnectionLine).start, (*tempConnectionLine).end);

    for (const auto& connection : connections)
    {
        auto start = mapFromGlobal(connection.output->globalConnectPointPosition());
        auto end = mapFromGlobal(connection.input->globalConnectPointPosition());
        drawPortConnection(painter, start, end);
    }
}

Object* WorkArea::object(const ObjectId id)
{
    return objects.at(id);
}

QUndoStack& WorkArea::undoHistory()
{
    return undoStack;
}

void WorkArea::sizeToContents()
{
    auto scrollArea = mainWindow()->workAreaScrollArea();
    auto scrollAreaSize = scrollArea->size();
    QSize newSize(
        scrollAreaSize.width() - scrollArea->verticalScrollBar()->size().width(),
        scrollAreaSize.height() - scrollArea->horizontalScrollBar()->size().height()
    );

    for (const auto widget : findChildren<QWidget*>())
    {
        const auto topLeft = widget->pos();
        const auto size = widget->size();
        const QPoint bottomRight(topLeft.x() + size.width(), topLeft.y() + size.height());

        if (bottomRight.x() + scrollAreaMargin > newSize.width()) newSize.setWidth(bottomRight.x() + scrollAreaMargin);
        if (bottomRight.y() + scrollAreaMargin > newSize.height()) newSize.setHeight(bottomRight.y() + scrollAreaMargin);
    }

    undoStack.push(new ResizeWorkAreaCommand(newSize, size()));
}

void WorkArea::dragEnterEvent(QDragEnterEvent* event)
{
    const auto mime = event->mimeData();

    if (mime->hasFormat(catalogEntryMimeFormat))
    {
        catalogDragEnter(event);
        return;
    }
    else if (mime->hasFormat(objectMimeFormat))
    {
        objectDragEnter(event);
        return;
    } else if (mime->hasFormat(outputPortMimeFormat))
    {
        outputPortDragEnter(event);
        return;
    }

    QWidget::dragEnterEvent(event);
}

void WorkArea::dragMoveEvent(QDragMoveEvent* event)
{
    auto mime = event->mimeData();

    if (mime->hasFormat(objectMimeFormat))
    {
        objectDragMove(event);
        return;
    }
    else if (mime->hasFormat(outputPortMimeFormat))
    {
        outputPortDragMove(event);
        return;
    }
}

void WorkArea::dropEvent(QDropEvent* event)
{
    const auto mime = event->mimeData();

    if (mime->hasFormat(catalogEntryMimeFormat))
    {
        catalogDrop(event);
        return;
    }
    else if (mime->hasFormat(objectMimeFormat))
    {
        objectDrop(event);
        return;
    }

    QWidget::dropEvent(event);
}

void WorkArea::objectDragEnter(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void WorkArea::objectDragMove(QDragMoveEvent* event)
{
    auto mime = qobject_cast<const ObjectMimeData*>(event->mimeData());
    auto object = mime->object;
    auto objectTopLeft = event->position().toPoint() - object->dragStartPosition();
    auto objectSize = object->size();
    auto scrollArea = mainWindow()->workAreaScrollArea();
    auto workAreaSize = size();
    bool resizeWorkArea = false;

    if (objectTopLeft.x() < 0) objectTopLeft.setX(0);
    if (objectTopLeft.y() < 0) objectTopLeft.setY(0);

    auto objectBottomRight = objectTopLeft + QPoint(objectSize.width(), objectSize.height());
    auto workAreaBottomRight = QPoint(workAreaSize.width(), workAreaSize.height());
    auto needGrow = objectBottomRight + QPoint(scrollAreaMargin, scrollAreaMargin) - workAreaBottomRight;

    if (needGrow.x() > 0)
    {
        workAreaSize.setWidth(workAreaSize.width() + needGrow.x());
        resizeWorkArea = true;
    }

    if (needGrow.y() > 0)
    {
        workAreaSize.setHeight(workAreaSize.height() + needGrow.y());
        resizeWorkArea = true;
    }

    if (resizeWorkArea)
    {
        setFixedWidth(workAreaSize.width());
        setFixedHeight(workAreaSize.height());
    }

    object->move(objectTopLeft);
    scrollArea->ensureWidgetVisible(object, scrollAreaMargin, scrollAreaMargin);
}

void WorkArea::objectDrop(QDropEvent* event)
{
    const auto mime = dynamic_cast<const ObjectMimeData*>(event->mimeData());
    auto object = mime->object;
    auto oldPosition = object->dragStartPosition();
    auto newPosition = event->position().toPoint() - oldPosition;

    object->dragEnd();

    undoStack.push(new MoveObjectCommand(object->id, newPosition, oldPosition));
}

void WorkArea::catalogDragEnter(QDragEnterEvent* event)
{
    const auto mime = dynamic_cast<const CatalogMimeData*>(event->mimeData());

    if (mime->entry.type() == catalogObjectItemType)
    {
        event->acceptProposedAction();
    }
}

void WorkArea::catalogDrop(QDropEvent* event)
{
    const auto mime = dynamic_cast<const CatalogMimeData*>(event->mimeData());
    const auto& entry = mime->entry;

    if (entry.type() == catalogObjectItemType)
    {
        event->acceptProposedAction();
        createObjectStart(dynamic_cast<const CatalogObjectItem&>(mime->entry).descriptor, event->position().toPoint());
    }
}

void WorkArea::createObjectStart(const Clypsalot::ObjectDescriptor& descriptor, const QPoint& position)
{
    assert(createObjectDialog == nullptr);

    createObjectDialog = new CreateObjectDialog(mainWindow(), descriptor, position);
    connect(createObjectDialog, SIGNAL(finished(int)), this, SLOT(createObjectFinished(int)));
    createObjectDialog->show();
}

void WorkArea::createObjectFinished(const int shouldAdd)
{
    assert(createObjectDialog != nullptr);

    if (shouldAdd)
    {
        const auto& descriptor = createObjectDialog->descriptor;
        const auto& config = createObjectDialog->config();
        const auto outputs = createObjectDialog->outputs();
        const auto inputs = createObjectDialog->inputs();
        const auto& position = createObjectDialog->position;
        undoStack.push(new AddObjectCommand(this, descriptor, config, outputs, inputs, position));
    }

    delete createObjectDialog;
    createObjectDialog = nullptr;
}

void WorkArea::addObject(Object* const object)
{
    assert(! objects.contains(object->id));

    objects[object->id] = object;
    object->show();
}

void WorkArea::removeObject(const size_t id)
{
    auto object = objects.at(id);

    object->setParent(nullptr);
    objects.erase(id);
    delete object;
}

void WorkArea::clearTempConnectionLine()
{
    if (tempConnectionLine != nullptr)
    {
        delete tempConnectionLine;
        tempConnectionLine = nullptr;
    }

    update();
}

void WorkArea::outputPortDragEnter(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void WorkArea::outputPortDragMove(QDragMoveEvent* event)
{
    auto mime = qobject_cast<const OutputPortMimeData*>(event->mimeData());
    auto port = mime->port;
    auto lineStart = mapFromGlobal(port->globalConnectPointPosition());

    if (tempConnectionLine != nullptr) delete tempConnectionLine;
    tempConnectionLine = new TempConnectionLine(lineStart, event->position().toPoint());
    update();
}

bool WorkArea::hasConnection(OutputPort* from, InputPort* to)
{
    for (const auto& connection : connections)
    {
        if (connection.output == from && connection.input == to) return true;
    }

    return false;
}

void WorkArea::connectPortsCommand(OutputPort* from, InputPort* to)
{
    if (hasConnection(from, to))
    {
        LOGGER(error, "Ports are already connected");
        return;
    }

    undoStack.push(new ConnectPortsCommand(from, to));
}

void WorkArea::addConnection(OutputPort* from, InputPort* to)
{
    assert(! hasConnection(from, to));

    connections.emplace_back(from, to);
    update();
}

void WorkArea::removeConnection(OutputPort* from, InputPort* to)
{
    assert(hasConnection(from, to));

    [[maybe_unused]] bool found = false;

    for (auto i = connections.begin(); i != connections.end();)
    {
        if (i->output == from && i->input == to)
        {
            i = connections.erase(i);
            found = true;
        }
        else i++;
    }

    assert(found);

    update();
}

void WorkArea::startObjects()
{
    for (auto [id, uiObject] : objects)
    {
        auto object = uiObject->object;
        std::unique_lock lock(*object);
        Clypsalot::startObject(object);
    }
}

AddObjectCommand::AddObjectCommand(
    WorkArea* const workArea,
    const Clypsalot::ObjectDescriptor& descriptor,
    const Clypsalot::ObjectConfig& config,
    const std::vector<std::pair<QString, QString>> addOutputs,
    const std::vector<std::pair<QString, QString>> addInputs,
    const QPoint& position
) :
    QUndoCommand(QString::fromStdString(Clypsalot::makeString("Add ", descriptor.kind, " object"))),
    workArea(workArea),
    descriptor(descriptor),
    config(config),
    addOutputs(addOutputs),
    addInputs(addInputs),
    position(position)
{ }

void AddObjectCommand::redo()
{
    Clypsalot::SharedObject object;

    objectId = nextObjectId++;

    LOGGER(verbose, "Creating ", descriptor.kind, " object with id=", objectId);

    {
        object = descriptor.make();
        std::unique_lock lock(*object);

        for (const auto& [type, name] : addOutputs) object->addOutput(type.toStdString(), name.toStdString());
        for (const auto& [type, name] : addInputs) object->addInput(type.toStdString(), name.toStdString());
        object->configure(config);
    }

    auto uiObject = new Object(objectId, object, workArea);

    uiObject->move(position);
    workArea->addObject(uiObject);
}

void AddObjectCommand::undo()
{
    LOGGER(verbose, "Removing object with id=", objectId);

    workArea->removeObject(objectId);
    nextObjectId--;
}

MoveObjectCommand::MoveObjectCommand(const ObjectId id, const QPoint& newPosition, const QPoint& oldPosition) :
    QUndoCommand("Move Object"),
    id(id),
    newPosition(newPosition),
    oldPosition(oldPosition)
{ }

void MoveObjectCommand::redo()
{
    auto object = mainWindow()->workArea()->object(id);
    object->move(newPosition);
    mainWindow()->workAreaScrollArea()->ensureWidgetVisible(object);
}

void MoveObjectCommand::undo()
{
    auto object = mainWindow()->workArea()->object(id);
    object->move(oldPosition);
    mainWindow()->workAreaScrollArea()->ensureWidgetVisible(object);
}

ResizeWorkAreaCommand::ResizeWorkAreaCommand(const QSize& newSize, const QSize& oldSize) :
    QUndoCommand("Resize Work Area"),
    newSize(newSize),
    oldSize(oldSize)
{ }

void ResizeWorkAreaCommand::redo()
{
    auto workArea = mainWindow()->workArea();
    workArea->setFixedWidth(newSize.width());
    workArea->setFixedHeight(newSize.height());
}

void ResizeWorkAreaCommand::undo()
{
    auto workArea = mainWindow()->workArea();
    workArea->setFixedWidth(oldSize.width());
    workArea->setFixedHeight(oldSize.height());
}

ConnectPortsCommand::ConnectPortsCommand(OutputPort* output, InputPort* input) :
    QUndoCommand(),
    output(output),
    input(input)
{ }

void ConnectPortsCommand::redo()
{
    auto fromObject = output->parentObject()->object;
    auto fromName = output->name().toStdString();
    auto toObject = input->parentObject()->object;
    auto toName = input->name().toStdString();
    std::unique_lock fromLock(*fromObject);
    std::unique_lock toLock(*toObject);

    Clypsalot::linkPorts(fromObject->output(fromName), toObject->input(toName));
    mainWindow()->workArea()->addConnection(output, input);
}

void ConnectPortsCommand::undo()
{
    auto fromObject = output->parentObject()->object;
    auto fromName = output->name().toStdString();
    auto toObject = input->parentObject()->object;
    auto toName = input->name().toStdString();
    std::unique_lock fromLock(*fromObject);
    std::unique_lock toLock(*toObject);

    Clypsalot::unlinkPorts(fromObject->output(fromName), toObject->input(toName));
    mainWindow()->workArea()->removeConnection(output, input);
}

void drawPortConnection(QPainter& painter, const QPoint& start, const QPoint& end)
{
    painter.drawLine(start, end);
}
