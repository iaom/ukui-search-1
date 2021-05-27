/*
 *
 * Copyright (C) 2020, KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Authors: zhangjiaping <zhangjiaping@kylinos.cn>
 *
 */
#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QScrollArea>
#include "home-page-section.h"
#include "file-utils.h"
#include "pluginmanage/search-plugin-manager.h"

namespace Zeeker {
class HomePage : public QScrollArea
{
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage() = default;

private:
    void initUi();
    void appendSection(HomePageSection *);
    QWidget * m_widget = nullptr;
    QVBoxLayout * m_mainLyt = nullptr;

    void registerSections();
    void createSection(const QString &, const HomePageItemShape &, QVector<HomePageItem>);
Q_SIGNALS:

};
}

#endif // HOMEPAGE_H