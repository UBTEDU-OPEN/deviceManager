#ifndef UPGRADABLEMODULESSELECTOR_H
#define UPGRADABLEMODULESSELECTOR_H

#include "utilsGlobal.h"

#include <QDialog>
#include <QJsonObject>
#include <QSet>
#include <QResizeEvent>

namespace Ui {
class UpgradableModulesSelector;
}

class UTILS_EXPORT UpgradableModulesSelector : public QDialog
{
    Q_OBJECT

public:
    explicit UpgradableModulesSelector(const QString &remoteGrpVersion,
                                       const QString &updateLog,
                                       const QMap<QString, QJsonObject> &upgradeModules,
                                       QWidget *parent = nullptr);
    ~UpgradableModulesSelector();

    void onIgnoreClicked();
    void onUpgradeClicked();

protected:
    void loadStyleSheet();
    void resizeEvent(QResizeEvent *event) override;

private:
    void initModulesTable(const QMap<QString, QJsonObject> &upgradeModules);
    QString convertModuleName(const QString& name);
    QString convertVersion(const QString& version);

signals:
    void ignored(const QString &remoteGrpVersion);
    void upgrade(const QString &remoteGrpVersion, const QMap<QString, QJsonObject> &upgradeModules);

private:
    Ui::UpgradableModulesSelector *ui;
    QSet<QString> upgradeModules_;
    QString remoteGrpVersion_;
};

#endif // UPGRADABLEMODULESSELECTOR_H
