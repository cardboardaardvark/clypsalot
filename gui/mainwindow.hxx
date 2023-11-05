/* Copyright 2023 Tyler Riddle
 *
 * This file is part of Clypsalot. Clypsalot is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version. Clypsalot is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Clypsalot. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <QMainWindow>
#include <QScrollArea>
#include <QUndoView>

#include "workarea.hxx"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow* ui = nullptr;
    QUndoView* undoHistoryWindow;

    void initEditMenu();
    bool shouldQuit();

    protected:
    virtual void closeEvent(QCloseEvent* event) override;

    Q_SIGNALS:
    void statusUpdate(const QString& message);

    public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    WorkArea* workArea();
    QScrollArea* workAreaScrollArea();

    public Q_SLOTS:
    void sizeWorkAreaToContents();
    void loadModules();
    void showLogWindow();
    void showUndoHistoryWindow();
    void redrawWorkArea();
    void startObjects();
};
