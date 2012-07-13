/*
 * Software License Agreement (BSD License)
 *
 *  Point Cloud Library (PCL) - www.pointclouds.org
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef PCL_MODELER_TREE_ITEM_H_
#define PCL_MODELER_TREE_ITEM_H_

#include <pcl/apps/modeler/qt.h>

class QMenu;
class QPoint;

namespace pcl
{
  namespace modeler
  {
    class MainWindow;

    class TreeItem : public QStandardItem
    {
      public:
        TreeItem(MainWindow* main_window);
        TreeItem(MainWindow* main_window, const QString & text);
        TreeItem(MainWindow* main_window, const QIcon & icon, const QString & text);
        virtual ~TreeItem();

        virtual TreeItem*
        parent();

        void
        updateOnDataChanged();

        virtual void
        updateOnAboutToBeInserted();

        virtual void
        updateOnAboutToBeRemoved();

        virtual void
        updateOnInserted();

        virtual void
        updateOnRemoved();

        virtual void
        updateOnSelectionChange(bool selected);

        void
        showContextMenu(const QPoint* position);

      protected:
        virtual void
        handleDataChange();

        virtual void
        prepareContextMenu(QMenu* menu) const;

        void
        forceChildCheckState(Qt::CheckState check_state);

        void
        updateParentCheckState();

      protected:
        MainWindow*     main_window_;
        QStandardItem*  old_state_;
    };
  }
}

#endif // PCL_MODELER_TREE_ITEM_H_