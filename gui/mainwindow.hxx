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
#include <QPushButton>
#include <QScrollArea>
#include <QUndoView>

#include "workarea.hxx"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Ui::MainWindow* m_ui = nullptr;

    bool shouldQuit();
    std::initializer_list<QPushButton*> selectionDependentButtons();

    protected:
    void closeEvent(QCloseEvent* event) override;

    Q_SIGNALS:
    void statusUpdate(const QString& message);

    public:
    static MainWindow* instance();
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    WorkArea* workArea();
    void errorMessage(const QString& message);
    void statusMessage(const QString& message);

    public Q_SLOTS:
    void loadModules();
    void showLogWindow();
    void workAreaSelectionChanged();
};
