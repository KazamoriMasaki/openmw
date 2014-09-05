#ifndef CSM_DOC_RUNNER_H
#define CSM_DOC_RUNNER_H

#include <QObject>
#include <QProcess>
#include <QTextDocument>

#include <components/esm/debugprofile.hpp>

class QTemporaryFile;

namespace CSMDoc
{
    class Runner : public QObject
    {
            Q_OBJECT

            QProcess mProcess;
            bool mRunning;
            ESM::DebugProfile mProfile;
            std::string mStartupInstruction;
            QTemporaryFile *mStartup;
            QTextDocument mLog;

        public:

            Runner();

            ~Runner();

            /// \param delayed Flag as running but do not start the OpenMW process yet (the
            /// process must be started by another call of start with delayed==false)
            void start (bool delayed = false);

            void stop();

            /// \note Running state is entered when the start function is called. This
            /// is not necessarily identical to the moment the child process is started.
            bool isRunning() const;

            void configure (const ESM::DebugProfile& profile,
                const std::string& startupInstruction);

            QTextDocument *getLog();

        signals:

            void runStateChanged();

        private slots:

            void finished (int exitCode, QProcess::ExitStatus exitStatus);

            void readyReadStandardOutput();
    };

    class Operation;

    /// \brief Watch for end of save operation and restart or stop runner
    class SaveWatcher : public QObject
    {
            Q_OBJECT

            Runner *mRunner;

        public:

            /// *this attaches itself to runner
            SaveWatcher (Runner *runner, Operation *operation);

        private slots:

            void saveDone (int type, bool failed);
    };
}

#endif
