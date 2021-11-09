/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
**
**  SysStat is a Qt-based interface to system statistics
**
**  Authors:
**       Copyright (c) 2009 - 2012 Kuzma Shapran <Kuzma.Shapran@gmail.com>
**
**  This library is free software; you can redistribute it and/or
**  modify it under the terms of the GNU Lesser General Public
**  License as published by the Free Software Foundation; either
**  version 2.1 of the License, or (at your option) any later version.
**
**  This library is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**  Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License along with this library;
**  if not, write to the Free Software Foundation, Inc.,
**  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** END_COMMON_COPYRIGHT_HEADER */


#ifndef LIBSYSSTAT__CPU_STAT__PRIVATE__INCLUDED
#define LIBSYSSTAT__CPU_STAT__PRIVATE__INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QtCore/QObject>
#include <QtCore/QtGlobal>
#include <QtCore/QMap>
#include <QtCore/QPair>

#include "basestat_p.h"
#include "cpustat.h"


namespace SysStat {

class CpuStatPrivate : public BaseStatPrivate
{
    Q_OBJECT

public:
    CpuStatPrivate(CpuStat *parent = nullptr);
    ~CpuStatPrivate() override;

    void updateSources();

    CpuStat::Monitoring monitoring() const;
    void setMonitoring(CpuStat::Monitoring value);

#ifdef HAVE_SYSCTL_H
    ulong minFreq(const QString &source) const;
    ulong maxFreq(const QString &source) const;
    ulong CurrentFreq(const QString);

#else
    uint minFreq(const QString &source) const;
    uint maxFreq(const QString &source) const;
#endif

signals:
    void update(float user, float nice, float system, float other);
    void update(uint frequency);
    void update(float user, float nice, float system, float other, float frequencyRate, uint frequency);

private slots:
    void timeout();

protected:
    void intervalChanged() override;
    void sourceChanged() override;

private:
    QString defaultSource() override;

    typedef struct Values
    {
        Values();

#ifdef HAVE_SYSCTL_H
        ulong user;
        ulong nice;
        ulong system;
        ulong idle;
        ulong other;
        ulong total;
#else
        qulonglong user;
        qulonglong nice;
        qulonglong system;
        qulonglong idle;
        qulonglong other;
        qulonglong total;
#endif

        void sum();

        void clear();
    } Values;
    Values mPrevious;

    CpuStat::Monitoring mMonitoring;

#ifdef HAVE_SYSCTL_H
    typedef QMap<QString, QPair<ulong, ulong> > Bounds;
    int mib0[2];
    int mib1[2];
    int mib2[512][4];
#else
    typedef QMap<QString, QPair<uint, uint> > Bounds;
#endif
    Bounds mBounds;

    int mUserHz;

    void addSource(const QString &source);

    float mIntervalMin;
    float mIntervalMax;

    void recalculateMinMax();
};

}

#endif //LIBSYSSTAT__CPU_STAT__PRIVATE__INCLUDED
