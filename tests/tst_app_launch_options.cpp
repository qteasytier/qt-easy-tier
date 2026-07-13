#include <QTest>

#include "app/AppLaunchOptions.h"

class TestAppLaunchOptions : public QObject {
    Q_OBJECT

private slots:
    void detectsAutoStartArgument()
    {
        QVERIFY(isAutoStartLaunch({QStringLiteral("QtEasyTier"), QStringLiteral("--autostart")}));
    }

    void returnsFalseWhenAutoStartArgumentIsMissing()
    {
        QVERIFY(!isAutoStartLaunch({QStringLiteral("QtEasyTier")}));
    }

    void doesNotMatchSimilarArguments()
    {
        QVERIFY(!isAutoStartLaunch({QStringLiteral("QtEasyTier"), QStringLiteral("--autostarted")}));
        QVERIFY(!isAutoStartLaunch({QStringLiteral("QtEasyTier"), QStringLiteral("autostart")}));
    }
};

QTEST_MAIN(TestAppLaunchOptions)
#include "tst_app_launch_options.moc"
