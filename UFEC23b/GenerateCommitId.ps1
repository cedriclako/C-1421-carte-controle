# The IDE shall add a ${ProjDirPath} environment variable.
# ensure to configure cube IDE in consequence

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

$gitFileOutput += "#define GITCOMMIT_COMMITID `"$gitCommitID`"`r`n"
$gitFileOutput += "#define GITCOMMIT_BRANCH `"$gitBranch`"`r`n"
$gitFileOutput += "#define GITCOMMIT_ISDIRTY $gitIsDirty`r`n"

$gitFileOutput += "`r`n"
$gitFileOutput += "#endif`r`n"

Write-Output $gitFileOutput | Out-File -Encoding utf8 ..\App\Inc\GitCommit.h

Exit 0
