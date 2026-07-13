#include <QTest>

#include "core/system_tray/TrayMessageDispatcher.h"
#include "core/system_tray/TrayMessageHelper.h"
#include "core/system_tray/TrayMessageSink.h"

class RecordingTrayMessageSink final : public TrayMessageSink {
public:
    void showTrayMessage(const TrayMessage &message) override
    {
        messages.append(message);
    }

    QList<TrayMessage> messages;
};

class TestTrayMessageDispatcher : public QObject {
    Q_OBJECT

private slots:
    void cleanup()
    {
        TrayMessageDispatcher::instance()->clearSinks();
    }

    void dispatchesMessageToRegisteredSink()
    {
        RecordingTrayMessageSink sink;
        auto *dispatcher = TrayMessageDispatcher::instance();
        dispatcher->addSink(&sink);

        dispatcher->showMessage({TrayMessageLevel::Info,
                                 QStringLiteral("标题"),
                                 QStringLiteral("内容"),
                                 1500});

        QCOMPARE(sink.messages.size(), 1);
        QCOMPARE(sink.messages.first().level, TrayMessageLevel::Info);
        QCOMPARE(sink.messages.first().title, QStringLiteral("标题"));
        QCOMPARE(sink.messages.first().message, QStringLiteral("内容"));
        QCOMPARE(sink.messages.first().durationMs, 1500);
    }

    void ignoresDuplicateSinkRegistration()
    {
        RecordingTrayMessageSink sink;
        auto *dispatcher = TrayMessageDispatcher::instance();
        dispatcher->addSink(&sink);
        dispatcher->addSink(&sink);

        dispatcher->showMessage({TrayMessageLevel::Warning,
                                 QStringLiteral("重复"),
                                 QStringLiteral("只应收到一次"),
                                 3000});

        QCOMPARE(sink.messages.size(), 1);
    }

    void removedSinkDoesNotReceiveMessages()
    {
        RecordingTrayMessageSink sink;
        auto *dispatcher = TrayMessageDispatcher::instance();
        dispatcher->addSink(&sink);
        dispatcher->removeSink(&sink);

        dispatcher->showMessage({TrayMessageLevel::Error,
                                 QStringLiteral("移除"),
                                 QStringLiteral("不应收到"),
                                 3000});

        QVERIFY(sink.messages.isEmpty());
    }

    void helperUsesDispatcherForAllLevels()
    {
        RecordingTrayMessageSink sink;
        TrayMessageDispatcher::instance()->addSink(&sink);

        TrayMessageHelper::showInfo(QStringLiteral("Info"), QStringLiteral("info message"), 1000);
        TrayMessageHelper::showWarning(QStringLiteral("Warning"), QStringLiteral("warning message"), 2000);
        TrayMessageHelper::showError(QStringLiteral("Error"), QStringLiteral("error message"), 3000);

        QCOMPARE(sink.messages.size(), 3);
        QCOMPARE(sink.messages.at(0).level, TrayMessageLevel::Info);
        QCOMPARE(sink.messages.at(0).title, QStringLiteral("Info"));
        QCOMPARE(sink.messages.at(0).durationMs, 1000);
        QCOMPARE(sink.messages.at(1).level, TrayMessageLevel::Warning);
        QCOMPARE(sink.messages.at(1).message, QStringLiteral("warning message"));
        QCOMPARE(sink.messages.at(1).durationMs, 2000);
        QCOMPARE(sink.messages.at(2).level, TrayMessageLevel::Error);
        QCOMPARE(sink.messages.at(2).title, QStringLiteral("Error"));
        QCOMPARE(sink.messages.at(2).durationMs, 3000);
    }
};

QTEST_MAIN(TestTrayMessageDispatcher)
#include "tst_tray_message_dispatcher.moc"
