/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Device Utilities module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QNETWORKSETTINGSSERVICE_H
#define QNETWORKSETTINGSSERVICE_H

#include <QObject>
#include "qnetworksettings.h"

QT_FORWARD_DECLARE_CLASS(QNetworkSettingsServicePrivate)

class Q_DECL_EXPORT QNetworkSettingsService : public QObject
{
    Q_OBJECT
    Q_ENUMS(StateTypes)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QNetworkSettingsState::States state READ state NOTIFY stateChanged)
    Q_PROPERTY(QNetworkSettingsType::Types type READ type NOTIFY typeChanged)
    Q_PROPERTY(QNetworkSettingsIPv4* ipv4 READ ipv4 NOTIFY ipv4Changed)
    Q_PROPERTY(QNetworkSettingsIPv6* ipv6 READ ipv6 NOTIFY ipv6Changed)
    Q_PROPERTY(QNetworkSettingsProxy* proxy READ proxy  NOTIFY proxyChanged)
    Q_PROPERTY(QNetworkSettingsWireless* wirelessConfig READ wirelessConfig NOTIFY wirelessChanged)
    Q_PROPERTY(QAbstractItemModel* domains READ domains NOTIFY domainsChanged)
    Q_PROPERTY(QAbstractItemModel* nameservers READ nameservers NOTIFY nameserversChanged)
public:
    explicit QNetworkSettingsService(const QString& aServiceId, QObject* parent = 0);

    QString id() const;
    QString name() const;
    QNetworkSettingsState::States state();
    QNetworkSettingsType::Types type();
    QNetworkSettingsIPv4* ipv4();
    QNetworkSettingsIPv6* ipv6();
    QNetworkSettingsProxy* proxy();
    QAbstractItemModel* domains();
    QAbstractItemModel* nameservers();
    QNetworkSettingsWireless* wirelessConfig();

    Q_INVOKABLE void setAutoConnect(const bool autoconnect);
    Q_INVOKABLE void setupIpv4Config();
    Q_INVOKABLE void setupIpv6Config();
    Q_INVOKABLE void setupNameserversConfig();
    Q_INVOKABLE void setupDomainsConfig();
    Q_INVOKABLE void setupNetworkSettingsProxy();
    //Wireless config
    Q_INVOKABLE void connectService();
    Q_INVOKABLE void disconnectService();
Q_SIGNALS:
    void nameChanged();
    void stateChanged();
    void typeChanged();
    void proxyChanged();
    void ipv4Changed();
    void ipv6Changed();
    void domainsChanged();
    void nameserversChanged();
    void wirelessChanged();
    void showCrendentialInput();
protected:
    QNetworkSettingsServicePrivate *d_ptr;

    Q_DISABLE_COPY(QNetworkSettingsService)
    Q_DECLARE_PRIVATE(QNetworkSettingsService)
};

#endif // QNETWORKSETTINGSSERVICE_H