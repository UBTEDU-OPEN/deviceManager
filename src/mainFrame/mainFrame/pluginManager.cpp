#include "pluginManager.h"

#include "commonClassroom.h"

#include "settings.h"
#include "config.h"
#include "logHelper.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QPluginLoader>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

PluginManager::PluginManager(QWidget *parent)
    : QDialog(parent)
{
    setVisible(false);
    setFixedWidth(360);
    mainLayout_ = new QVBoxLayout(this);
    loadPlugins();
}

PluginManager::~PluginManager()
{
}

void PluginManager::loadPlugins()
{
    // to do: add plugins info on server
    // local plugins：
    auto pluginsPath = Settings::pluginsAbsPath();
    QMap<QString, QString> pluginName2version = Config::pluginsInfo();
    for (auto itr = pluginName2version.begin(); itr != pluginName2version.end(); ++itr) {
        auto name = itr.key();
        auto version = itr.value();
        auto pluginPath = pluginsPath + "/" + name + "/" + version;
        auto pluginDir = QDir(pluginPath);
        const auto entryList = pluginDir.entryList(QDir::Files);
        for (const QString &fileName : entryList) {
            auto loader = new QPluginLoader(pluginDir.absoluteFilePath(fileName));
            if (auto plugin = qobject_cast<DevPluginInterface*>(loader->instance())) {
                QJsonObject json = loader->metaData().value("MetaData").toObject();
                LOG(INFO) << "";
                LOG(INFO) << "********** MetaData **********";
                LOG(INFO) << json.value("author").toString().toStdString();
                LOG(INFO) << json.value("date").toString().toStdString();
                LOG(INFO) << json.value("name").toString().toStdString();
                LOG(INFO) << json.value("version").toString().toStdString();
                LOG(INFO) << json.value("device_type").toInt(0);
                auto pluginInfo = new PluginInfo(json.value("name").toString(), json.value("version").toString(),
                                                 "http://la.lsl.s", "10.0.0");
                mainLayout_->addWidget(pluginInfo);

                plugin->setAuthor(json.value("author").toString());
                plugin->setDate(json.value("date").toString());
                plugin->setVersion(json.value("version").toString());
                plugin->setName(json.value("name").toString());
                plugin->setDeviceType(json.value("device_type").toInt(0));
                plugins.insert(json.value("name").toString(), plugin);
            }
        }
    }
    mainLayout_->addStretch(1);
}

DevPluginUI *PluginManager::load(const QString &classroomFile)
{
    QFile file(classroomFile);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray classroomData = file.readAll();
        file.close();
        QJsonParseError jsonParseError;
        auto jsonObj = QJsonDocument::fromJson(classroomData, &jsonParseError).object();
        QString pluginName = jsonObj[ClassroomFileKey::DeviceTypeName].toString();
        if (plugins.contains(pluginName)) {
            return plugins[pluginName]->createUI(classroomFile);
        }
    }
    return nullptr;
}

DevPluginUI* PluginManager::createNewClassroom(const QString& platformName)
{
    if (plugins.contains(platformName)) {
        return plugins[platformName]->createUI("");
    }
    return nullptr;
}

void PluginManager::setLanguage(Language lan)
{
    for (auto  plugin : plugins) {
        plugin->setLanguage(lan);
    }
}

PluginInfo::PluginInfo(const QString &pluginName, const QString &localVersion,
                       const QString &url, const QString &/*remoteVersion*/, QWidget *parent)
    : QWidget(parent)
    , url_(url)
{
    auto mainLayout = new QHBoxLayout(this);
    auto label = new QLabel(pluginName);
    mainLayout->addWidget(label);
    mainLayout->addStretch(1);
    label = new QLabel(localVersion);
    mainLayout->addWidget(label);
    auto btn = new QPushButton;
    btn->setText(tr("download"));
    mainLayout->addWidget(btn);
    connect(btn, &QPushButton::clicked, this, [pluginName, url]() {
        // download plugin
        LOG(INFO) << "download " << pluginName.toStdString() << " url:" << url.toStdString();
    });
}

PluginInfo::~PluginInfo()
{

}

