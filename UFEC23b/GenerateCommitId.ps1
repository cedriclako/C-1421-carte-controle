# The IDE shall add a ${ProjDirPath} environment variable.
# ensure to configure cube IDE in consequence
$path = $env:Path

if ([string]::IsNullOrEmpty($env:ProjDirPath)) {
	Write-Error "Environment variable is not set 'ProjDirPath'. Add it to project properties."
	Exit 1
}

# --------------------------
# Final file
# --------------------------

$gitCommitID = $(git describe --always --long --tags)
$gitBranch = $(git rev-parse --abbrev-ref HEAD)
$gitIsDirty = 1

if ([string]::IsNullOrEmpty($(git status --porcelain))) {
	$gitIsDirty = 0;
}

$gitFileOutput = "#ifndef _GITCOMMIT_ID_H_`r`n"
$gitFileOutput += "#define _GITCOMMIT_ID_H_`r`n"
$gitFileOutput += "`r`n"

$gitFileOutput += "#define GITCOMMIT_COMMITID `"$gitBranch`"`r`n"
$gitFileOutput += "#define GITCOMMIT_BRANCH `"$gitCommitID`"`r`n"
$gitFileOutput += "#define GITCOMMIT_ISDIRTY $gitIsDirty`r`n"

$gitFileOutput += "`r`n"
$gitFileOutput += "#endif`r`n"

Write-Output $gitFileOutput | Out-File -Encoding utf8 $env:ProjDirPath\App\Inc\GitCommit.h

Exit 0
