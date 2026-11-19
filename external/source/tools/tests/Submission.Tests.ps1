$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-ModWorkflow.ps1')
. (Join-Path $PSScriptRoot '../GC-SubmitMission.ps1')
function Assert($value,$message) {if(!$value){throw $message}}
$fixture=Join-Path ([IO.Path]::GetTempPath()) ('gc-submission-test-'+[guid]::NewGuid())
$upstream=Join-Path $fixture 'shared';$author=Join-Path $fixture 'author';$forkPath=Join-Path $fixture 'personal.git'
$null=New-Item -ItemType Directory -Path $upstream -Force
$null=Git $upstream @('init','-q','-b','dev')
$null=Git $upstream @('config','user.name','Fixture')
$null=Git $upstream @('config','user.email','fixture@example.invalid')
$worldPath='worlds/Example.ent';$headerPath='Missions/Example.conf'
$null=New-Item -ItemType Directory -Path "$upstream/worlds/Example_Layers","$upstream/Missions" -Force
[IO.File]::WriteAllText("$upstream/addon.gproj",('GameProject {'+[Environment]::NewLine+' GUID "62D55563549DA97B"'+[Environment]::NewLine+'}'))
[IO.File]::WriteAllText("$upstream/$worldPath",'base world')
[IO.File]::WriteAllText("$upstream/$worldPath.meta",'MetaFileClass {'+"`n"+' Name "{ABCD123456789012}worlds/Example.ent"'+"`n"+'}')
[IO.File]::WriteAllText("$upstream/$headerPath",'GC_MissionHeader {'+"`n"+' World "{ABCD123456789012}worlds/Example.ent"'+"`n"+' m_sName "COOP Alpha"'+"`n"+'}')
[IO.File]::WriteAllText("$upstream/$headerPath.meta",'header meta')
[IO.File]::WriteAllText("$upstream/worlds/Example_Layers/default.layer",'base layer')
[IO.File]::WriteAllText("$upstream/readme.txt",'shared readme')
$null=Git $upstream @('add','.')
$null=Git $upstream @('commit','-qm','Base')
$null=Git $upstream @('clone','-q',$upstream,$author)
$null=Git $upstream @('clone','-q','--bare',$upstream,$forkPath)
$null=Git $author @('config','user.name','Author')
$null=Git $author @('config','user.email','author@example.invalid')
$repo='Global-Conflicts-ArmA/gc-reforger-missions'
$null=Git $author @('config',"url.$upstream.insteadOf","https://github.com/$repo.git")
$null=Git $author @('config',"url.$forkPath.insteadOf",'https://tester@github.com/tester/gc-reforger-missions.git')
$addons=@(@{Name='GC Missions';Key='GlobalConflictsMissions';Project="$author/addon.gproj";Guid='62D55563549DA97B';Repo=$repo})
$script:forkReady=$false;$script:created=0;$script:patched=0;$script:pr=$null;$script:cancel=$false;$script:choice='mine';$script:conflicts=0;$script:reviewMutation=$null
function Get-SubmissionAuth {@{}}
function Get-GcGitHubStatus { @{Connected=$true} }
function Get-GcAddons { $addons }
$SubmitMission=$true
function Set-GcOutcome($Status,$Detail,$Url) { $script:outcome=@{Status=$Status;Detail=$Detail;Url=$Url} }
function Show-GcReview($spec) {
    if ($spec.Title -eq 'Resolve conflicting file') {
        $script:conflicts++
        Assert ($spec.Fields[0].ReadOnly -and $spec.Fields[1].ReadOnly) 'Conflict preview must be read-only'
        if ($script:choice -eq 'cancel') {return}
        return @{Action=$script:choice;Values=@{}}
    }
    if ($spec.Section -eq 'Submit for approval') {
        $expected=if($script:pr.state -eq 'open'){'Update submission'}else{'Submit'}
        Assert ($spec.PrimaryAction -eq $expected) 'Submission action is unclear'
        Assert (@($spec.Fields | Where-Object Kind -eq filelist).Count -ge 1) 'Changed files not reviewed'
        if ($script:verifyFiles) {
            Assert (($spec.Fields | Where-Object Id -eq filesA).Value -match 'new dependency.txt') 'Added file missing from review'
            Assert (($spec.Fields | Where-Object Id -eq filesM).Value -match 'worlds/Example.ent') 'Changed file missing from review'
            Assert (($spec.Fields | Where-Object Id -eq filesD).Value -eq 'readme.txt') 'Deleted file missing from review'
            $script:verifyFiles=$false
        }
        if ($script:cancel) {return}
        if ($script:reviewMutation) {& $script:reviewMutation;$script:reviewMutation=$null}
        return @{Action='confirm';Values=@{title='COOP Alpha';overview='Mission changes';tested0=$true;reviewer='Check objectives';dependencies='None'}}
    }
    throw "Unexpected review: $($spec.Title)"
}
function Invoke-SubmissionApi($auth,$method,$path,$data,[switch]$AllowMissing) {
    if($path -eq 'user'){return @{login='tester';id=42}}
    if($path -eq 'repos/tester/gc-reforger-missions') {if($script:forkReady){return @{fork=$true;parent=@{full_name=$repo};default_branch='dev'}};return}
    if($method -eq 'POST' -and $path -eq "repos/$repo/forks") {$script:forkReady=$true;return @{full_name='tester/gc-reforger-missions'}}
    if($method -eq 'GET' -and $path.Contains('/pulls?')){
        if($script:closeAfterPush -and $path.Contains('state=open&')) {
            $script:openChecks++
            if($script:openChecks -eq 4){$script:pr.state='closed';$script:closeAfterPush=$false}
        }
        $unrelated=@{number=9000;state=$(if($path.Contains('state=open&')){'open'}else{'closed'});head=@{repo=@{full_name='another-user/gc-reforger-missions'};ref='unrelated-branch'};base=@{ref='dev'}}
        $page=@($unrelated)
        if($script:pr -and $path.Contains('state='+$script:pr.state+'&')) {$page+=($script:pr | ConvertTo-Json -Depth 10 | ConvertFrom-Json)}
        # Match Windows PowerShell Invoke-RestMethod's non-enumerated JSON array.
        return ,$page
    }
    if($method -eq 'POST' -and $path.EndsWith('/pulls')) {
        $script:created++;$script:branch=$data.head.Substring('tester:'.Length)
        Assert ($data.base -eq 'dev' -and $data.maintainer_can_modify) 'Wrong PR destination'
        $main=Git $upstream @('rev-parse','HEAD')
        Assert ((Git $forkPath @('merge-base',$script:branch,$main)) -eq $main) 'PR created before latest main was merged'
        $script:pr=@{number=(11+$script:created);state='open';merged_at=$null;body=$data.body;title=$data.title;mergeable=$true;head=@{repo=@{full_name='tester/gc-reforger-missions';owner=@{login='tester'}};ref=$script:branch};base=@{ref='dev'}}
        return $script:pr
    }
    if($path -match '/pulls/\d+$') {
        if($method -eq 'PATCH'){Assert (!$data.state) 'Mission request state must never be changed';$script:patched++;$script:pr.body=$data.body;$script:pr.title=$data.title;$script:pr.base.ref=$data.base;if($data.state){$script:pr.state=$data.state}}
        if($script:closeOnRead -and $method -eq 'GET'){$script:pr.state='closed';$script:closeOnRead=$false}
        return $script:pr
    }
    throw "Unexpected API: $method $path"
}
function Assert-Synced {
    $main=Git $upstream @('rev-parse','HEAD')
    Assert ((Git $author @('merge-base','HEAD',$main)) -eq $main) 'Local branch does not contain main'
    Assert ((Git $author @('rev-parse','HEAD')) -eq (Git $forkPath @('rev-parse',$script:branch))) 'Local and submitted branch differ'
    Assert (!(Git $author @('status','--porcelain'))) 'Checkout was not clean after submission'
}
$initial=Git $author @('rev-parse','HEAD')
[IO.File]::WriteAllText("$author/$worldPath",'my first world')
[IO.File]::WriteAllText("$author/new dependency.txt",'mission dependency')
[IO.File]::WriteAllText("$upstream/shared.txt",'latest main')
$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Shared change')
[IO.File]::Delete("$author/readme.txt")
$script:verifyFiles=$true
$script:cancel=$true
Assert ((Invoke-ModWorkflow) -eq 2) 'Cancel not honored'
Assert ($script:outcome.Status -eq 'Cancelled') 'True cancellation lost outcome'
Assert ((Git $author @('rev-parse','HEAD')) -eq $initial -and $created -eq 0 -and !$forkReady) 'Cancel mutated branch or GitHub'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'my first world') 'Cancel changed files'
$script:cancel=$false
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Initial submission failed'
Assert ($created -eq 1 -and $script:branch -like 'mission/submission-*') 'Automatic branch/PR missing'
Assert ((Git $author @('rev-parse','dev')) -eq $initial) 'Original main branch was modified'
Assert ((Git $author @('branch','--show-current')) -eq $script:branch) 'Automatic mission branch not selected'
Assert ((Git $forkPath @('show',($script:branch+':new dependency.txt'))) -eq 'mission dependency') 'Saved new files missing'
Assert (!(Test-Path "$author/readme.txt")) 'Deleted file restored during submission'
Assert-Synced
Assert (@(Git $author @('for-each-ref','--format=%(refname)','refs/gc/backups')).Count -ge 2) 'Recovery refs missing'
$script:pr.base.ref='main' # An existing release-targeted request must be reused and retargeted.
[IO.File]::WriteAllText("$author/$worldPath",'local conflict')
[IO.File]::WriteAllText("$upstream/$worldPath",'shared conflict')
$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Shared conflict')
$before=Git $author @('rev-parse','HEAD');$remoteBefore=Git $forkPath @('rev-parse',$script:branch)
$script:choice='cancel'
Assert ((Invoke-MissionSubmission $addons) -eq 2) 'Conflict cancel not honored'
Assert ((Git $author @('rev-parse','HEAD')) -eq $before -and (Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Conflict cancellation changed a branch'
$script:choice='mine'
Assert ((Invoke-MissionSubmission $addons) -eq 0 -and $conflicts -ge 2 -and $created -eq 1 -and $patched -eq 1) 'Conflict resolution/PR reuse failed'
Assert ($script:pr.base.ref -eq 'dev') 'Existing request was not retargeted to dev'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'local conflict') 'Mine choice lost'
Assert-Synced
[IO.File]::WriteAllText("$author/$worldPath",'local next')
[IO.File]::WriteAllText("$upstream/$worldPath",'shared next')
$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Shared next')
$script:choice='shared'
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Shared choice failed'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'shared next') 'Shared choice not applied locally'
Assert-Synced
[IO.File]::WriteAllText("$author/$worldPath",'before review')
$before=Git $author @('rev-parse','HEAD');$remoteBefore=Git $forkPath @('rev-parse',$script:branch)
$script:reviewMutation={ [IO.File]::WriteAllText("$author/$worldPath",'edited during review') }
try {$null=Invoke-MissionSubmission $addons;throw 'Stale local review accepted'} catch {Assert ($_.Exception.Message -eq 'Your files changed during review. Submit again.') $_.Exception.Message}
Assert ((Git $author @('rev-parse','HEAD')) -eq $before -and (Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Stale review uploaded'
$script:reviewMutation={ [IO.File]::WriteAllText("$upstream/shared.txt",'changed during review');$null=Git $upstream @('add','.');$null=Git $upstream @('commit','-qm','Moving main') }
try {$null=Invoke-MissionSubmission $addons;throw 'Stale main accepted'} catch {Assert ($_.Exception.Message -like 'Shared missions changed during review.*') $_.Exception.Message}
[IO.File]::WriteAllText("$forkPath/hooks/pre-receive", "#!/bin/sh`nexit 1`n", (New-Object Text.UTF8Encoding($false)))
try {$null=Invoke-MissionSubmission $addons;throw 'Rejected push accepted'} catch {Assert ($_.Exception.Message -like 'Upload stopped.*') $_.Exception.Message}
Assert ((Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Rejected push changed remote'
[IO.File]::Delete("$forkPath/hooks/pre-receive")
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Retry failed'
Assert-Synced
# Advance the personal branch independently, then resolve its conflict locally.
$online=Join-Path $fixture 'online'
$null=Git $upstream @('clone','-q','--branch',$script:branch,$forkPath,$online)
$null=Git $online @('config','user.name','Online')
$null=Git $online @('config','user.email','online@example.invalid')
[IO.File]::WriteAllText("$online/$worldPath",'online conflict')
[IO.File]::WriteAllText("$online/online.txt",'remote addition')
$null=Git $online @('add','.')
$null=Git $online @('commit','-qm','Online changes')
$onlineTip=Git $online @('rev-parse','HEAD')
$null=Git $online @('push','-q','origin',$script:branch)
[IO.File]::WriteAllText("$author/$worldPath",'local divergent conflict')
$conflictsBefore=$script:conflicts
$script:choice='shared'
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Divergent online submission failed'
Assert ($script:conflicts -gt $conflictsBefore) 'Online conflict was not reviewed'
Assert ([IO.File]::ReadAllText("$author/$worldPath") -eq 'online conflict') 'Online choice lost'
Assert ([IO.File]::ReadAllText("$author/online.txt") -eq 'remote addition') 'Online addition lost'
Assert ((Git $author @('merge-base','HEAD',$onlineTip)) -eq $onlineTip) 'Online history overwritten'
Assert-Synced
# Supporting files on a different local branch still update the open mission request.
$null=Git $author @('switch','-q','-c','another-local-branch')
[IO.File]::WriteAllText("$author/worlds/support.txt",'supporting file')
$firstRequest=$script:pr.number
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Supporting file failed'
Assert ($created -eq 1 -and $script:pr.number -eq $firstRequest) 'Supporting file created another request'
Assert ((Git $forkPath @('show',($script:branch+':worlds/support.txt'))) -eq 'supporting file') 'Supporting file absent from shared request'
Assert-Synced
# Returning to the shared checkout must reuse the same local submission branch.
[IO.File]::WriteAllText("$author/local-pending.txt",'not uploaded yet')
$null=Git $author @('add','local-pending.txt')
$null=Git $author @('commit','-qm','Local pending work')
$branchesBefore=@(Git $author @('for-each-ref','--format=%(refname)','refs/heads'))
$null=Git $author @('switch','-q','dev')
[IO.File]::WriteAllText("$author/worlds/notes.txt",'saved notes')
$script:normalGit=${function:Git}
function Git([string]$root,[string[]]$arguments) {
    if($arguments[0] -eq 'merge' -and $arguments -contains '--ff-only'){throw 'Fixture local apply failure'}
    & $script:normalGit $root $arguments
}
try {
    try {$null=Invoke-MissionSubmission $addons;throw 'Local apply failure accepted'} catch {Assert ($_.Exception.Message -like 'The update could not finish.*') $_.Exception.Message}
} finally {Set-Item function:Git $script:normalGit}
Assert ((Git $author @('branch','--show-current')) -eq 'dev') 'Apply failure did not restore source branch'
Assert ([IO.File]::ReadAllText("$author/worlds/notes.txt") -eq 'saved notes') 'Apply failure lost saved files'
[IO.File]::AppendAllText("$author/.git/info/exclude","`nlocal-pending.txt`n")
[IO.File]::WriteAllText("$author/local-pending.txt",'ignored local file')
try {$null=Invoke-MissionSubmission $addons;throw 'Ignored collision accepted'} catch {Assert ($_.Exception.Message -like 'The update could not finish.*') $_.Exception.Message}
Assert ([IO.File]::ReadAllText("$author/local-pending.txt") -eq 'ignored local file') 'Branch switch overwrote ignored file'
Assert ([IO.File]::ReadAllText("$author/worlds/notes.txt") -eq 'saved notes') 'Ignored collision lost saved files'
[IO.File]::Delete("$author/local-pending.txt")
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Submission from main failed'
Assert ([IO.File]::ReadAllText("$author/local-pending.txt") -eq 'not uploaded yet') 'Existing local pending work lost'
Assert ((Git $author @('branch','--show-current')) -eq $script:branch) 'Existing local submission branch not reused'
Assert ((@(Git $author @('for-each-ref','--format=%(refname)','refs/heads')) -join '|') -eq ($branchesBefore -join '|')) 'Resubmission created another branch'
Assert ([IO.File]::ReadAllText("$author/worlds/support.txt") -eq 'supporting file') 'Supporting file lost returning from main'
Assert ([IO.File]::ReadAllText("$author/worlds/notes.txt") -eq 'saved notes') 'New supporting file lost returning from main'
Assert ($created -eq 1) 'Submission from main duplicated request'
Assert-Synced
# A second mission must not be bundled into the open request.
[IO.File]::WriteAllText("$author/worlds/SecondMission.ent",'supporting file')
$before=Git $author @('rev-parse','HEAD');$remoteBefore=Git $forkPath @('rev-parse',$script:branch)
try {$null=Invoke-MissionSubmission $addons;throw 'Multiple missions accepted'} catch {Assert ($_.Exception.Message -like 'Submit one mission at a time.*') $_.Exception.Message}
Assert ((Git $author @('rev-parse','HEAD')) -eq $before -and (Git $forkPath @('rev-parse',$script:branch)) -eq $remoteBefore) 'Multiple missions mutated a branch'
[IO.File]::Delete("$author/worlds/SecondMission.ent")
# Closed requests and their remote branches remain untouched.
$script:pr.state='closed'
$closedPr=$script:pr;$closedBranch=$script:branch
$closedTip=Git $forkPath @('rev-parse',$closedBranch)
$patchedBefore=$script:patched
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Fresh submission after closure failed'
Assert ($created -eq 2 -and $script:pr.number -ne $firstRequest -and $script:branch -ne $closedBranch) 'Closed request or branch reused'
Assert ($closedPr.state -eq 'closed' -and $script:patched -eq $patchedBefore -and (Git $forkPath @('rev-parse',$closedBranch)) -eq $closedTip) 'Closed submission was modified'
$script:reviewMutation={ $script:pr.state='closed' }
$before=Git $author @('rev-parse','HEAD')
try {$null=Invoke-MissionSubmission $addons;throw 'Closure during review accepted'} catch {Assert ($_.Exception.Message -eq 'Your submission changed during review. Submit again.') $_.Exception.Message}
Assert ((Git $author @('rev-parse','HEAD')) -eq $before) 'Closure during review mutated local branch'
$script:closeOnRead=$true
Assert ((Invoke-ModWorkflow) -eq 2) 'Closure after resubmit reported success'
Assert ($script:outcome.Status -eq 'Resubmit') 'Closed request displayed awaiting approval'
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Retry after late closure stuck'
$script:closeAfterPush=$true;$script:openChecks=0
[IO.File]::WriteAllText("$author/worlds/support.txt",'updated support')
try {$null=Invoke-MissionSubmission $addons;throw 'Closure during upload accepted'} catch {Assert ($_.Exception.Message -like 'Your approval request changed during upload.*') $_.Exception.Message}
Assert ($script:pr.state -eq 'closed') 'Closure during upload duplicated or reopened request'
Assert-Synced
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Retry after upload closure failed'
# Approval finishes this request. Later work reuses the fork with a fresh branch,
# creating only the next request, never trying to reopen a merged request.
$null=Git $upstream @('fetch','-q',$forkPath,$script:branch)
$null=Git $upstream @('merge','--ff-only','-q','FETCH_HEAD')
$createdBefore=$created;$mergedBranch=$script:branch;$mergedTip=Git $forkPath @('rev-parse',$mergedBranch)
$script:pr.state='closed';$script:pr.merged_at='2026-11-13T12:00:00Z'
[IO.File]::WriteAllText("$author/worlds/ThirdMission.ent",'third mission')
Assert ((Invoke-MissionSubmission $addons) -eq 0) 'Post-approval submission failed'
Assert ($created -eq ($createdBefore+1) -and $script:pr.number -ne $firstRequest -and $script:pr.state -eq 'open') 'Post-approval request lifecycle incorrect'
Assert ($script:branch -ne $mergedBranch -and (Git $forkPath @('rev-parse',$mergedBranch)) -eq $mergedTip) 'Merged branch was reused or changed'
Assert-Synced
Write-Output "PASS: one mission per request, closed/merged branch isolation, closure races, Git-only submission, automatic branch, saved changes/dependencies, latest-main merge, both conflict choices, no mutation on cancel/stale review, backup refs, safe retry, existing PR reuse. Fixture: $fixture"
