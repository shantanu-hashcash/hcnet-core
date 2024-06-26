// Copyright 2015 Hcnet Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "historywork/ResolveSnapshotWork.h"
#include "history/StateSnapshot.h"
#include "ledger/LedgerManager.h"
#include "main/Application.h"
#include "util/GlobalChecks.h"
#include <Tracy.hpp>

namespace hcnet
{

ResolveSnapshotWork::ResolveSnapshotWork(
    Application& app, std::shared_ptr<StateSnapshot> snapshot)
    : BasicWork(app, "resolve-snapshot", BasicWork::RETRY_NEVER)
    , mSnapshot(snapshot)
{
    if (!mSnapshot)
    {
        throw std::runtime_error("ResolveSnapshotWork: invalid snapshot");
    }
}

BasicWork::State
ResolveSnapshotWork::onRun()
{
    ZoneScoped;
    mSnapshot->mLocalState.prepareForPublish(mApp);
    mSnapshot->mLocalState.resolveAnyReadyFutures();

    // We delay resolving snapshots by 1 ledger when not running standalone, in
    // case the node diverged, to avoid publishing bad data to an archive.
    bool pastPossibleConservativePublicationDelay =
        mApp.getConfig().RUN_STANDALONE ||
        mApp.getLedgerManager().getLastClosedLedgerNum() >
            mSnapshot->mLocalState.currentLedger;

    if (pastPossibleConservativePublicationDelay &&
        mSnapshot->mLocalState.futuresAllResolved())
    {
        releaseAssert(mSnapshot->mLocalState.containsValidBuckets(mApp));
        return State::WORK_SUCCESS;
    }
    else
    {
        setupWaitingCallback(std::chrono::seconds(1));
        return State::WORK_WAITING;
    }
}
}
