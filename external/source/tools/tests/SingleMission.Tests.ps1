$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '../GC-SubmitMission.ps1')
function Assert($value,$message) {if(!$value){throw $message}}
function Git($root,$arguments) {
    Assert ($arguments[0] -eq 'show') 'Unexpected Git operation'
    if($arguments[1] -like '*Deleted.conf') {
        Assert ($arguments[1] -like 'base:*') 'Deleted header not read from base'
        return ' World "{ABCD123456789012}worlds/Other.ent"'
    }
    Assert ($arguments[1] -like 'tree:*') 'Current header not read from candidate tree'
    ' World "{ABCD123456789012}worlds/Example.ent"'
}
function Files($paths) {@($paths | ForEach-Object {@{Status='M';Path=$_}})}
Assert-SingleMissionSubmission 'fixture' 'base' 'tree' (Files @('worlds/Example.ent','worlds/Example.ent.meta','worlds/Example_Layers/default.layer','Missions/Example.conf','scripts/support.c'))
Assert-SingleMissionSubmission 'fixture' 'base' 'tree' (Files @('worlds/Example_Layers/default.layer','worlds/EXAMPLE_Layers/nested/other.layer'))
Assert-SingleMissionSubmission 'fixture' 'base' 'tree' (Files @('scripts/support.c'))
foreach($paths in @(
    @('worlds/Example.ent','worlds/Other.ent'),
    @('worlds/Example_Layers/default.layer','worlds/Other_Layers/default.layer'),
    @('Missions/Example.conf','worlds/Other.ent.meta')
)) {
    try {Assert-SingleMissionSubmission 'fixture' 'base' 'tree' (Files $paths);throw 'Multiple missions accepted'}
    catch {Assert ($_.Exception.Message -like 'Submit one mission at a time.*') $_.Exception.Message}
}
try {
    Assert-SingleMissionSubmission 'fixture' 'base' 'tree' @(@{Status='D';Path='Missions/Deleted.conf'},@{Status='M';Path='Missions/Example.conf'})
    throw 'Deleted mission bundled with another mission'
} catch {Assert ($_.Exception.Message -like 'Submit one mission at a time.*') $_.Exception.Message}
Write-Output 'PASS: one world with layers/metadata/header/support, case normalization, multiple worlds, layer-only changes, header-only changes, deleted headers.'
