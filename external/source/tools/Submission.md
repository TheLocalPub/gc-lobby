# Mission submission

GC Tools submits saved GC Missions files for approval through GitHub. It does not
save or export a Workbench world, inspect the editor, or require Workbench running.
Save your work before selecting Submit.

## Account

Git Credential Manager handles GitHub browser sign-in. GC Tools uses the same
credential provider for Git and API requests; it has no password form. Submission
creates a personal fork when needed, so contributors need no upstream write access.

## Development targets

GC Core, GC Units and GC Missions checks, updates and pull requests target their
explicit `dev` branches. Missing dev is an error; release/default branches are
never substituted. Source checkout validation uses the separate Dev addon GUIDs.
GC Lobby continues to follow its own default branch.

## Workflow

1. Read the current branch and saved, unignored files in the GC Missions checkout.
2. Fetch the latest shared dev and any existing personal submission branch.
3. Combine those changes, resolving conflicts in the hub before uploading.
4. Review the mission name, overview, testing checklist, reviewer comments
   and Added / Changed / Deleted file lists. The form shows the
   supplied submission rules and testing-guide address. Only checks the author
   selects are marked complete in GitHub's Markdown checklist. Existing request
   text and checks are restored; a new request after closure or merge starts unchecked.
5. Save recovery references and a backup of local changes, then apply the reviewed
   result to the open request's branch, or a fresh mission submission branch
   when there is no open request.
6. Push without force to the contributor's fork and create or reuse its pull request.

All saved changes in that checkout are included, including dependency declarations
and supporting files. GC Tools does not infer or download missing mission assets.
The shared dev branch is never pushed by submission. The local mission branch
contains the fetched dev before upload. Cancellation during review leaves the
checkout unchanged. Recovery references live under refs/gc/backups.

## Units / loadouts first

Submit mission checks the GC Units source checkout first, including saved files,
local submission commits and the online submission branch. If it has pending
changes against the shared version, a separate units/loadouts review appears
before the mission form, with Added / Changed / Deleted lists and conflict review.
No pending changes means no units review, fork creation or upload.

The same submission engine handles both repositories. Each repository has its
own personal fork and one pending request. Confirming
units creates or updates its request; the mission form then shows that request
read-only and adds a Required units / loadouts link automatically to its PR body.
There is no manual Dependencies form. The units request and saved files are
rechecked before mission upload; a closed/conflicting or changed dependency stops
submission for another review. Merged units with no remaining changes need no
pending dependency link.

Cancelling the units step stops before mission upload. If units were uploaded and
the mission step is cancelled or fails, the units request remains saved and its
link is shown. Retrying reuses it. These two GitHub requests are not an atomic
transaction; reviewers still approve each separately and should merge units first.

## One mission, one pull request

One personal fork is reused. With no open mission request, Submit creates a fresh
`mission/submission-<unique id>` branch and a new pull request. Closed and merged
requests are complete: they are never reopened, and their branches are not reused
or fetched into a new submission. Saved files in the current checkout still form
the new submission; closing a request does not discard local work.

An open request is updated on its existing branch, including requests from older
tool versions. Lookup uses the contributor's fork, not the local editing branch.
Starting from another branch combines saved changes with that open submission.
Existing release-targeted requests are retargeted to dev after review. More than
one open request stops submission so a reviewer can resolve the ambiguity.

Only one mission world may be included in a submission. Changed world files,
world metadata, layer folders and mission-header World references identify the
world; changes for multiple worlds stop before checkout mutation or upload.
Keep other missions on separate branches, and finish or close the open request
before submitting another mission. Supporting scripts, assets and dependency
files remain included in the file review. This is a saved-file check, not an
editor query or dependency scanner.

Units / loadouts keep their existing reusable branch and closed-request resubmit
behavior; the fresh-PR policy applies to mission submissions.

Request state is rechecked around upload and after the API update. A request
closed or merged during the operation is never reported as awaiting approval
when the final check observes that state. External changes after that check are
picked up on the next submission; the displayed result is not a live GitHub feed.

## Conflicts and retries

Readable text conflicts show My version and Shared version with explicit file
choices. Binary, oversized and unsupported file conflicts stop for manual help;
GC Tools does not guess a winner. Changed local files, online submissions or dev
invalidate a review. A rejected upload keeps the local result for a safe retry.

GitHub can change after any check. Pushing to an existing pull request immediately
updates its head. If dev changes during upload, GC Tools requests another Submit
to synchronize it; that check cannot undo an already uploaded head. GitHub's final
mergeability check may still be pending when the request is created.

## Verification

Run external/source/tools/tests/Submission.Tests.ps1 with Windows PowerShell.
It uses isolated local Git repositories and mocked GitHub requests; no live pull
requests or user checkout mutations occur. Live contributor permissions remain
subject to GitHub's responses.
