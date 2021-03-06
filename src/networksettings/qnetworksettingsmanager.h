/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Device Utilities module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QNETWORKSETTINGSMANAGER_H
#define QNETWORKSETTINGSMANAGER_H

#include <QtNetworkSettings/qnetworksettings.h>
#include <QObject>
#include <QStringListModel>

QT_FORWARD_DECLARE_CLASS(QNetworkSettingsManagerPrivate)
QT_FORWARD_DECLARE_CLASS(QNetworkSettingsService)
QT_FORWARD_DECLARE_CLASS(QNetworkSettingsServiceModel)
QT_FORWARD_DECLARE_CLASS(QNetworkSettingsUserAgent)
QT_FORWARD_DECLARE_CLASS(QNetworkSettingsServiceFilter)
QT_FORWARD_DECLARE_CLASS(QNetworkSettingsInterface)
QT_FORWARD_DECLARE_CLASS(QNetworkSettingsInterfaceModel)

QT_BEGIN_NAMESPACE

class Q_DECL_EXPORT QNetworkSettingsManager : public QObject
{
    Q_OBJECT
    Q_ENUMS(StateTypes NetworkTypeTypes)
    Q_PROPERTY(QNetworkSettingsServiceFilter* services READ services NOTIFY servicesChanged)
    Q_PROPERTY(QNetworkSettingsInterfaceModel* interfaces READ interfaces NOTIFY interfacesChanged)
    Q_PROPERTY(QNetworkSettingsUserAgent* userAgent READ userAgent CONSTANT)
    Q_PROPERTY(QNetworkSettingsService* currentWifiConnection READ currentWifiConnection NOTIFY currentWifiConnectionChanged)
    Q_PROPERTY(QNetworkSettingsService* currentWiredConnection READ currentWiredConnection NOTIFY currentWiredConnectionChanged)
    Q_PROPERTY(QString usbEthernetIpAddress READ usbEthernetInternetProtocolAddress NOTIFY usbEthernetInternetProtocolAddressChanged)
    Q_PROPERTY(QString usbEthernetProtocol READ usbVirtualEthernetLinkProtocol NOTIFY usbVirtualEthernetLinkProtocolChanged)

public:
    explicit QNetworkSettingsManager(QObject* parent = Q_NULLPTR);
    QNetworkSettingsServiceFilter* services();
    QNetworkSettingsInterfaceModel* interfaces();
    void setUserAgent(QNetworkSettingsUserAgent *agent);
    QNetworkSettingsUserAgent* userAgent();

    Q_INVOKABLE QNetworkSettingsService* service(const QString& name, int type);
    Q_INVOKABLE void connectBySsid(const QString& name, const QString &passphrase);
    void clearConnectionState();
    void tryNextConnection();
    void clearCurrentConnection(QNetworkSettingsService* service);
    void setCurrentConnection(QNetworkSettingsService* service);
    QNetworkSettingsService* currentWifiConnection();
    QNetworkSettingsService* currentWiredConnection();
    Q_INVOKABLE QNetworkSettingsInterface* interface(int type, int instance);
    Q_INVOKABLE QString usbEthernetInternetProtocolAddress();
    Q_INVOKABLE QString usbVirtualEthernetLinkProtocol();
    Q_INVOKABLE bool hasUsbEthernetProtocolConfiguration();
    Q_INVOKABLE void setUsbVirtualEthernetLinkProtocol(const QString &protocol);

Q_SIGNALS:
    void servicesChanged();
    void interfacesChanged();
    void currentWifiConnectionChanged();
    void currentWiredConnectionChanged();
    void usbEthernetInternetProtocolAddressChanged(const QString &newusbEthernetIpAddress);
    void usbVirtualEthernetLinkProtocolChanged(const QString &newUsbEthernetProtocol);

protected:
    QNetworkSettingsManagerPrivate *d_ptr;

private:
    Q_DISABLE_COPY(QNetworkSettingsManager)
    Q_DECLARE_PRIVATE(QNetworkSettingsManager)

};

QT_END_NAMESPACE

#endif // QNETWORKSETTINGSMANAGER_H
