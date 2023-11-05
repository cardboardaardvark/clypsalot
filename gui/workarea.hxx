#pragma once

#include <QUndoStack>
#include <QWidget>

#include <clypsalot/module.hxx>

#include "createobjectdialog.hxx"
#include "object.hxx"

struct PortConnection
{
    OutputPort* output;
    InputPort* input;
};

struct TempConnectionLine
{
    const QPoint start;
    const QPoint end;
};

class WorkArea : public QWidget
{
    Q_OBJECT

    CreateObjectDialog* createObjectDialog = nullptr;
    QUndoStack undoStack;
    std::map<ObjectId, Object*> objects;
    std::vector<PortConnection> connections;
    struct TempConnectionLine* tempConnectionLine = nullptr;

    protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    void catalogDragEnter(QDragEnterEvent* event);
    void objectDragEnter(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event) override;
    void objectDragMove(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event) override;
    void catalogDrop(QDropEvent* event);
    void objectDrop(QDropEvent* event);
    void createObjectStart(const Clypsalot::ObjectDescriptor& descriptor, const QPoint& position);
    void outputPortDragEnter(QDragEnterEvent* event);
    void outputPortDragMove(QDragMoveEvent* event);

    protected Q_SLOTS:
    void createObjectFinished(const int shouldAdd);

    public:
    explicit WorkArea(QWidget *parent = nullptr);
    void sizeToContents();
    QUndoStack& undoHistory();
    Object* object(const ObjectId id);
    void addObject(Object* const object);
    void removeObject(const size_t id);
    bool hasConnection(OutputPort* from, InputPort* to);
    void connectPortsCommand(OutputPort* from, InputPort* to);
    void addConnection(OutputPort* from, InputPort* to);
    void removeConnection(OutputPort* from, InputPort* to);
    void startObjects();

    public Q_SLOTS:
    void clearTempConnectionLine();
};

class AddObjectCommand : public QUndoCommand
{
    static ObjectId nextObjectId;
    WorkArea* const workArea;
    ObjectId objectId = 0;
    const Clypsalot::ObjectDescriptor& descriptor;
    const Clypsalot::ObjectConfig config;
    const std::vector<std::pair<QString, QString>> addOutputs;
    const std::vector<std::pair<QString, QString>> addInputs;
    const QPoint position;

    public:
    AddObjectCommand(
            WorkArea* const workArea,
            const Clypsalot::ObjectDescriptor& descriptor,
            const Clypsalot::ObjectConfig& config,
            const std::vector<std::pair<QString, QString>> addOutputs,
            const std::vector<std::pair<QString, QString>> addInputs,
            const QPoint& position
    );
    virtual void redo() override;
    virtual void undo() override;
};

class MoveObjectCommand : public QUndoCommand
{
    const ObjectId id;
    const QPoint newPosition;
    const QPoint oldPosition;

    public:
    MoveObjectCommand(const ObjectId id, const QPoint& newPosition, const QPoint& oldPosition);
    virtual void redo() override;
    virtual void undo() override;
};

class ResizeWorkAreaCommand : public QUndoCommand
{
    const QSize newSize;
    const QSize oldSize;

    public:
    ResizeWorkAreaCommand(const QSize& newSize, const QSize& oldSize);
    virtual void redo();
    virtual void undo();
};

class ConnectPortsCommand : public QUndoCommand
{
    OutputPort* const output;
    InputPort* const input;

    public:
    ConnectPortsCommand(OutputPort* output, InputPort* input);
    virtual void redo() override;
    virtual void undo() override;
};

void drawPortConnection(QPainter& painter, const QPoint& start, const QPoint& end);
