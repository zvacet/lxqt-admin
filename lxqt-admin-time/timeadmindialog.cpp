/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 */

#include "timeadmindialog.h"
#include "ui_timeadmindialog.h"
#include <QFile>
#include <QDebug>
#include <QMessageBox>

#define ZONETAB_PATH     "/usr/share/zoneinfo/zone.tab"

TimeAdminDialog::TimeAdminDialog():
    QDialog(),
    mConfig(OOBS_TIME_CONFIG(oobs_time_config_get()))
{
    ui = new Ui::TimeAdminDialog;
    ui->setupUi(this);

    loadTimeZones();

    ui->time->setTime(QTime::currentTime());
    ui->calendar->showToday();

}

TimeAdminDialog::~TimeAdminDialog()
{
    if(mConfig)
        g_object_unref(mConfig);
    delete ui;
}

void TimeAdminDialog::loadTimeZones()
{
    QStringList timeZones;
    QFile file(ZONETAB_PATH);
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray line;
        while(!file.atEnd())
        {
            line = file.readLine().trimmed();
            if(line.isEmpty() || line[0] == '#') // skip comments or empty lines
                continue;
            QList<QByteArray> items = line.split('\t');
            if(items.length() <= 2)
                continue;
            timeZones.append(QLatin1String(items[2]));
        }
        file.close();
    }
    timeZones.sort();
    ui->timeZone->addItems(timeZones);
    
    int sel = -1;
    const char* currentTimeZone = oobs_time_config_get_timezone(mConfig);
    if(currentTimeZone)
        sel = timeZones.indexOf(QLatin1String(currentTimeZone));
    ui->timeZone->setCurrentIndex(sel);
}

void TimeAdminDialog::accept()
{
    // relly apply the settings
    GError* err = NULL;
    if(oobs_object_authenticate(OOBS_OBJECT(mConfig), &err))
    {
        QByteArray timeZone = ui->timeZone->currentText().toLatin1();
        // FIXME: currently timezone settings does not work. is this a bug of system-tools-backend?
        if(!timeZone.isEmpty())
            oobs_time_config_set_timezone(mConfig, timeZone.constData());
        QTime t = ui->time->time();
        QDate d = ui->calendar->selectedDate();
        // oobs seesm to use 0 based month
        oobs_time_config_set_time(mConfig, d.year(), d.month() - 1, d.day(), t.hour(), t.minute(), t.second());
        oobs_object_commit(OOBS_OBJECT(mConfig));
    }
    else if(err)
    {
        QMessageBox::critical(this, tr("Authentication Error"), QString::fromUtf8(err->message));
        g_error_free(err);
    }
    QDialog::accept();
}
