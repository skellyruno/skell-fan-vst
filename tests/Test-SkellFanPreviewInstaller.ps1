[CmdletBinding()]
param([Parameter(Mandatory)][string]$PackageRoot)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$script:root=[IO.Path]::GetFullPath($PackageRoot)
$script:installer=Join-Path $script:root 'Install-SkellFanPreview.ps1'
$script:count=0
$temp=Join-Path $env:TEMP ('SkellFan-Test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp|Out-Null
function Assert($ok,$message){if(!$ok){throw "Assertion failed: $message"};$script:count++;Write-Host "PASS: $message"}
function Throws([scriptblock]$action,[string]$match,[string]$message){$caught=$false;try{& $action|Out-Null}catch{$caught=$true;if($match-and $_.ToString() -notmatch [regex]::Escape($match)){throw "Unexpected error: $($_.ToString())"}};if(!$caught){throw "Expected failure: $message"};$script:count++;Write-Host "PASS: $message"}
function RunInstaller([hashtable]$argsMap){& $script:installer @argsMap -Confirm:$false -ErrorAction Stop}
function CopyPackage([string]$dest){New-Item -ItemType Directory -Path $dest|Out-Null;foreach($n in @('SkellFan.vst3','build-info.json','SHA256SUMS.txt','Install-SkellFanPreview.ps1','INSTALL_PREVIEW.md')){Copy-Item -LiteralPath (Join-Path $script:root $n) -Destination $dest -Recurse}}
function TreeHash([string]$path){$r=[IO.Path]::GetFullPath($path).TrimEnd([char[]]@('\','/'));$f=@(Get-ChildItem -LiteralPath $r -File -Force -Recurse|Sort-Object FullName);$rows=foreach($x in $f){$rel=$x.FullName.Substring($r.Length).TrimStart([char[]]@('\','/')).Replace('\','/');$rel+'  '+(Get-FileHash $x.FullName -Algorithm SHA256).Hash.ToLowerInvariant()};$sha=[Security.Cryptography.SHA256]::Create();try{[Convert]::ToHexString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(($rows -join [char]10)+[char]10))).ToLowerInvariant()}finally{$sha.Dispose()}}
try{
 $savedLocalAppData=[Environment]::GetEnvironmentVariable('LOCALAPPDATA','Process')
 try {
  [Environment]::SetEnvironmentVariable('LOCALAPPDATA',$null,'Process')
  Throws {RunInstaller @{PackageRoot=$script:root;WhatIf=$true}} 'LOCALAPPDATA must be an absolute path' 'Missing LOCALAPPDATA rejects implicit defaults'
  Throws {RunInstaller @{PackageRoot=$script:root;InstallRoot=(Join-Path $temp 'partial-install');WhatIf=$true}} 'LOCALAPPDATA must be an absolute path' 'Partial custom roots fail closed without LOCALAPPDATA'
  $customInstall=Join-Path $temp 'custom-install';$customData=Join-Path $temp 'custom-data'
  $null=RunInstaller @{PackageRoot=$script:root;InstallRoot=$customInstall;DataRoot=$customData;WhatIf=$true}
  Assert (!(Test-Path $customInstall)-and !(Test-Path $customData)) 'Both explicit custom roots work without LOCALAPPDATA in WhatIf'
 } finally {
  [Environment]::SetEnvironmentVariable('LOCALAPPDATA',$savedLocalAppData,'Process')
 }
 $wi=Join-Path $temp 'wi';$wd=Join-Path $temp 'wd';$null=RunInstaller @{PackageRoot=$script:root;InstallRoot=$wi;DataRoot=$wd;WhatIf=$true};Assert (!(Test-Path $wi)-and !(Test-Path $wd)) 'WhatIf is read-only'
 $drive=[IO.Path]::GetPathRoot([IO.Path]::GetFullPath($env:TEMP));Throws {RunInstaller @{PackageRoot=$script:root;InstallRoot=$drive;DataRoot=(Join-Path $temp 'rootdata');Approve=$true}} 'filesystem root' 'Filesystem root rejected'
 $junction=Join-Path $temp 'package-junction'
 New-Item -ItemType Junction -Path $junction -Target $script:root|Out-Null
 try {
  Throws {RunInstaller @{PackageRoot=$junction;InstallRoot=(Join-Path $temp 'ji');DataRoot=(Join-Path $temp 'jd');WhatIf=$true}} 'Reparse point' 'Package-root junction rejected before verification'
 } finally {Remove-Item -LiteralPath $junction -Force}

 $occupied=Join-Path $temp 'occupied';$bundle=Join-Path $occupied 'SkellFan.vst3';New-Item -ItemType Directory -Path $bundle -Force|Out-Null;$sentinel=Join-Path $bundle 'keep.txt';Set-Content $sentinel 'keep'
 Throws {RunInstaller @{PackageRoot=$script:root;InstallRoot=$occupied;DataRoot=(Join-Path $temp 'occupieddata');Approve=$true}} 'not owned' 'Unowned existing install refused'
 Assert ((Get-Content $sentinel -Raw).Trim() -eq 'keep') 'Existing data preserved'
 $tamper=Join-Path $temp 'tamper';CopyPackage $tamper;$bin=Join-Path $tamper 'SkellFan.vst3\Contents\x86_64-win\SkellFan.vst3';$bytes=[IO.File]::ReadAllBytes($bin);$bytes[128]=$bytes[128]-bxor 1;[IO.File]::WriteAllBytes($bin,$bytes)
 Throws {RunInstaller @{PackageRoot=$tamper;InstallRoot=(Join-Path $temp 'ti');DataRoot=(Join-Path $temp 'td');Approve=$true}} 'Binary/moduleinfo hash differs from provenance' 'Tampered package refused'
 $duplicate=Join-Path $temp 'duplicate-json';CopyPackage $duplicate
 $duplicateInfo=Join-Path $duplicate 'SkellFan.vst3\Contents\Resources\moduleinfo.json'
 $duplicateText=[IO.File]::ReadAllText($duplicateInfo)
 $duplicateText=$duplicateText -replace '"Name"\s*:\s*"SkellFan"','"Name":"SkellFan","name":"SkellFan"'
 [IO.File]::WriteAllText($duplicateInfo,$duplicateText,[Text.UTF8Encoding]::new($false))
 $duplicateInstall=Join-Path $temp 'duplicate-install'
 Throws {RunInstaller @{PackageRoot=$duplicate;InstallRoot=$duplicateInstall;DataRoot=(Join-Path $temp 'duplicate-data');WhatIf=$true}} 'Duplicate or case-ambiguous JSON key' 'Case-ambiguous moduleinfo keys rejected'
 Assert (!(Test-Path $duplicateInstall)) 'Rejected duplicate metadata causes no install mutation'
 $nanPackage=Join-Path $temp 'nan-json';CopyPackage $nanPackage
 $nanManifest=Join-Path $nanPackage 'build-info.json';$nanText=[IO.File]::ReadAllText($nanManifest).TrimEnd()
 $nanText=$nanText.Substring(0,$nanText.Length-1)+',"unexpected":NaN}'+[Environment]::NewLine
 [IO.File]::WriteAllText($nanManifest,$nanText,[Text.UTF8Encoding]::new($false))
 Throws {RunInstaller @{PackageRoot=$nanPackage;InstallRoot=(Join-Path $temp 'nan-install');DataRoot=(Join-Path $temp 'nan-data');WhatIf=$true}} 'Invalid strict JSON' 'Non-standard NaN provenance value rejected'

 $install=Join-Path $temp 'install';$data=Join-Path $temp 'data';$result=RunInstaller @{PackageRoot=$script:root;InstallRoot=$install;DataRoot=$data;Approve=$true};$target=Join-Path $install 'SkellFan.vst3';$statePath=Join-Path $data 'install-state.json'
 Assert ($result.Status -eq 'Installed'-and(Test-Path $statePath)) 'Fresh per-user install succeeds'
 $same=RunInstaller @{PackageRoot=$script:root;InstallRoot=$install;DataRoot=$data;Approve=$true};Assert ($same.Status -eq 'AlreadyCurrent') 'Install is idempotent'
 $modulePath=Join-Path $target 'Contents\Resources\moduleinfo.json';$module=Get-Content $modulePath -Raw|ConvertFrom-Json -AsHashtable;$module.Version='1.8.0';foreach($c in $module.Classes){$c.Version='1.8.0'};[IO.File]::WriteAllText($modulePath,(ConvertTo-Json -InputObject $module -Depth 8)+[char]10,[Text.UTF8Encoding]::new($false))
 $oldState=Get-Content $statePath -Raw|ConvertFrom-Json -AsHashtable;$oldState.version='1.8.0';$oldState.bundle_sha256=TreeHash $target;[IO.File]::WriteAllText($statePath,(ConvertTo-Json -InputObject $oldState -Depth 8)+[char]10,[Text.UTF8Encoding]::new($false))
 $updated=RunInstaller @{PackageRoot=$script:root;InstallRoot=$install;DataRoot=$data;Approve=$true};Assert ($updated.Status -eq 'Updated'-and(Test-Path (Join-Path $updated.BackupPath 'backup-manifest.json'))) 'Update creates verified backup'
 $badJsonBackup=Join-Path $data 'Backups\BadJsonBackup'
 Copy-Item -LiteralPath $updated.BackupPath -Destination $badJsonBackup -Recurse
 $backupManifest=Join-Path $badJsonBackup 'backup-manifest.json'
 $backupText=[IO.File]::ReadAllText($backupManifest)
 $backupText=$backupText -replace '"product"\s*:\s*"SkellFan"','"product":"SkellFan","Product":"SkellFan"'
 [IO.File]::WriteAllText($backupManifest,$backupText,[Text.UTF8Encoding]::new($false))
 $beforeBadJsonRollback=TreeHash $target
 Throws {RunInstaller @{PackageRoot=$script:root;InstallRoot=$install;DataRoot=$data;RollbackBackup=$badJsonBackup;Approve=$true}} 'Duplicate or case-ambiguous JSON key' 'Case-ambiguous rollback manifest rejected'
 Assert ((TreeHash $target) -eq $beforeBadJsonRollback) 'Rejected rollback manifest leaves installed bundle unchanged'
 $rollback=RunInstaller @{PackageRoot=$script:root;InstallRoot=$install;DataRoot=$data;RollbackBackup=$updated.BackupPath;Approve=$true};$restored=Get-Content $statePath -Raw|ConvertFrom-Json -AsHashtable;Assert ($rollback.Status -eq 'RolledBack'-and$restored.version -eq '1.8.0') 'Rollback restores prior state'
 $before=(Get-FileHash (Join-Path $target 'Contents\x86_64-win\SkellFan.vst3') -Algorithm SHA256).Hash;$bad=Join-Path $data 'Backups\BadBackup';Copy-Item -LiteralPath $updated.BackupPath -Destination $bad -Recurse;$badBin=Join-Path $bad 'SkellFan.vst3\Contents\x86_64-win\SkellFan.vst3';$b=[IO.File]::ReadAllBytes($badBin);$b[128]=$b[128]-bxor 1;[IO.File]::WriteAllBytes($badBin,$b)
 Throws {RunInstaller @{PackageRoot=$script:root;InstallRoot=$install;DataRoot=$data;RollbackBackup=$bad;Approve=$true}} 'checksum verification failed' 'Tampered backup refused'
 $after=(Get-FileHash (Join-Path $target 'Contents\x86_64-win\SkellFan.vst3') -Algorithm SHA256).Hash;Assert ($before -eq $after) 'Rejected rollback leaves install unchanged'
 Write-Host "SkellFan installer integration: $script:count assertions passed."
}finally{if(Test-Path $temp){Remove-Item -LiteralPath $temp -Recurse -Force}}
