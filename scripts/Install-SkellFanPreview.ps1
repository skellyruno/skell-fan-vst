[CmdletBinding(SupportsShouldProcess=$true,ConfirmImpact='High')]
param(
 [string]$PackageRoot=$PSScriptRoot,
 [string]$InstallRoot=(Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'),
 [string]$DataRoot=(Join-Path $env:LOCALAPPDATA 'Skell Audio\SkellFan'),
 [string]$RollbackBackup,
 [switch]$Approve
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
function Full([string]$p) { [IO.Path]::GetFullPath($p) }
function Hash([string]$p) { (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant() }
function TreeHash([string]$p) {
 $root=(Full $p).TrimEnd([char[]]@('\','/'))
 $files=@(Get-ChildItem -LiteralPath $root -File -Force -Recurse | Sort-Object FullName)
 if(!$files.Count){throw "Empty tree: $p"}
 $rows=foreach($f in $files){$rel=$f.FullName.Substring($root.Length).TrimStart([char[]]@('\','/')).Replace('\','/');$rel+'  '+(Hash $f.FullName)}
 $sha=[Security.Cryptography.SHA256]::Create()
 try{[Convert]::ToHexString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(($rows -join [char]10)+[char]10))).ToLowerInvariant()}finally{$sha.Dispose()}
}
function SafePath([string]$p) {
 $full=Full $p;$root=[IO.Path]::GetPathRoot($full);$trim=$full.TrimEnd([char[]]@('\','/'))
 if($trim -eq $root.TrimEnd([char[]]@('\','/'))){throw "Refusing filesystem root: $full"}
 foreach($protected in @($env:ProgramFiles,[Environment]::GetEnvironmentVariable('ProgramFiles(x86)'),$env:WINDIR,$env:SystemRoot)|Where-Object{$_}){
  $q=(Full $protected).TrimEnd([char[]]@('\','/'))
  if($trim.Equals($q,[StringComparison]::OrdinalIgnoreCase)-or $trim.StartsWith($q+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw "Refusing system directory: $full"}
 }
 $segments=$full.Substring($root.Length).Split([char[]]@('\','/'),[StringSplitOptions]::RemoveEmptyEntries);$cur=$root
 foreach($s in $segments){$cur=[IO.Path]::Combine($cur,$s);if(Test-Path -LiteralPath $cur){if((Get-Item -LiteralPath $cur -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse point: $cur"}}}
 $full
}
function NoLinks([string]$p) {
 foreach($i in Get-ChildItem -LiteralPath $p -Force -Recurse -ErrorAction Stop){if($i.Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse point in package: $($i.FullName)"}}
}
function Json([string]$p) { Get-Content -LiteralPath $p -Raw | ConvertFrom-Json -AsHashtable -ErrorAction Stop }
function SaveJson([string]$p,$v) {
 $parent=Split-Path -Parent $p
 if(!(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent -Force|Out-Null}
 $tmp=$p+'.tmp-'+[guid]::NewGuid().ToString('N')
 [IO.File]::WriteAllText($tmp,(ConvertTo-Json -InputObject $v -Depth 8)+[char]10,[Text.UTF8Encoding]::new($false))
 Move-Item -LiteralPath $tmp -Destination $p -Force
}
function ValidateBundle([string]$b,[string]$version) {
 NoLinks $b
 $bin=Join-Path $b 'Contents\x86_64-win\SkellFan.vst3';$mi=Join-Path $b 'Contents\Resources\moduleinfo.json'
 if(!(Test-Path -LiteralPath $bin -PathType Leaf)-or !(Test-Path -LiteralPath $mi -PathType Leaf)){throw 'VST3 module/moduleinfo missing'}
 $m=Json $mi
 if($m.Name -ne 'SkellFan'-or $m.Version -ne $version-or $m['Factory Info'].Vendor -ne 'Skell Audio'){throw 'VST3 identity mismatch'}
 $classes=@($m.Classes);$ids=@($classes|ForEach-Object{$_.CID}|Sort-Object -Unique)
 $cats=@($classes|ForEach-Object{$_.Category})
 if($classes.Count -lt 2-or $ids.Count -ne $classes.Count-or !$cats.Contains('Audio Module Class')-or !$cats.Contains('Component Controller Class')){throw 'VST3 class inventory invalid'}
 $bytes=[IO.File]::ReadAllBytes($bin)
 if($bytes.Length -lt 64-or $bytes[0] -ne 77-or $bytes[1] -ne 90){throw 'PE MZ signature missing'}
 $o=[BitConverter]::ToInt32($bytes,60)
 if($o -lt 64-or $o+26 -gt $bytes.Length-or $bytes[$o] -ne 80-or $bytes[$o+1] -ne 69-or [BitConverter]::ToUInt16($bytes,$o+4) -ne 0x8664-or [BitConverter]::ToUInt16($bytes,$o+24) -ne 0x20b){throw 'Expected AMD64 PE32+ VST3'}
 [pscustomobject]@{Binary=$bin;ModuleInfo=$mi}
}
function ValidatePackage {
 $r=Full $PackageRoot
 NoLinks $r
 $expected=@('SkellFan.vst3','build-info.json','SHA256SUMS.txt','Install-SkellFanPreview.ps1','INSTALL_PREVIEW.md')|Sort-Object
 $actual=@(Get-ChildItem -LiteralPath $r -Force|Select-Object -ExpandProperty Name|Sort-Object)
 if(Compare-Object $expected $actual){throw 'Package top-level inventory mismatch'}
 $m=Json (Join-Path $r 'build-info.json')
 if($m.product -ne 'SkellFan'-or $m.vendor -ne 'Skell Audio'-or $m.format -ne 'VST3'-or $m.platform -ne 'windows-x64'-or $m.release_status -ne 'preview-only'){throw 'Invalid preview provenance'}
 if($m.validation.pluginval.result -ne 'passed'-or [int]$m.validation.pluginval.strictness_level -lt 5){throw 'pluginval strictness 5 provenance missing'}
 if($m.source_commit -notmatch '^[0-9a-f]{40}$'-or $m.ci_checkout_commit -notmatch '^[0-9a-f]{40}$'){throw 'Source/CI provenance missing'}
 $b=Join-Path $r 'SkellFan.vst3';$v=ValidateBundle $b $m.version
 if((Hash $v.Binary) -ne $m.binary.sha256-or (Hash $v.ModuleInfo) -ne $m.moduleinfo.sha256){throw 'Binary/moduleinfo hash differs from provenance'}
 $expectedPaths=@('SkellFan.vst3/Contents/x86_64-win/SkellFan.vst3','SkellFan.vst3/Contents/Resources/moduleinfo.json','build-info.json','Install-SkellFanPreview.ps1','INSTALL_PREVIEW.md')
 $seen=@{}
 foreach($line in Get-Content -LiteralPath (Join-Path $r 'SHA256SUMS.txt') -Encoding ascii){
  if($line -notmatch '^([0-9a-fA-F]{64})  (.+)$'){throw 'Malformed SHA256SUMS entry'}
  $h=$Matches[1].ToLowerInvariant();$rel=$Matches[2]
  if($rel -match '(^/|\\|:|(^|/)\.\.?(/|$)|//)'){throw "Unsafe checksum path: $rel"}
  if($seen.ContainsKey($rel)){throw "Duplicate checksum path: $rel"}
  $f=Full (Join-Path $r $rel.Replace('/',[IO.Path]::DirectorySeparatorChar))
  $prefix=$r.TrimEnd([char[]]@('\','/'))+[IO.Path]::DirectorySeparatorChar
  if(!$f.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)-or !(Test-Path -LiteralPath $f -PathType Leaf)){throw "Unsafe or missing checksum target: $rel"}
  if((Hash $f) -ne $h){throw "SHA-256 mismatch: $rel"}
  $seen[$rel]=$h
 }
 if($seen.Count -ne 5){throw 'SHA256SUMS must cover exactly five files'}
 foreach($p in $expectedPaths){if(!$seen.ContainsKey($p)){throw "SHA256SUMS missing: $p"}}
 [pscustomobject]@{Root=$r;Bundle=$b;BundleHash=(TreeHash $b);Manifest=$m}
}
function State([string]$target,[string]$statePath) {
 $exists=Test-Path -LiteralPath $target;$hasState=Test-Path -LiteralPath $statePath
 if(!$exists){if($hasState){throw 'Install state exists but bundle is missing'};return $null}
 NoLinks $target
 if(!$hasState){throw 'Existing SkellFan installation is not owned by this installer; refusing overwrite'}
 $s=Json $statePath
 if($s.product -ne 'SkellFan'-or $s.install_path -ine (Full $target)-or $s.bundle_sha256 -ne (TreeHash $target)){throw 'Existing installation does not match verified install state'}
 ValidateBundle $target $s.version|Out-Null
 $s
}
function Backup([string]$target,[string]$statePath,[string]$backupRoot,$s) {
 New-Item -ItemType Directory -Path $backupRoot -Force|Out-Null
 $b=Join-Path $backupRoot ([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssfff')+'-'+[guid]::NewGuid().ToString('N'))
 New-Item -ItemType Directory -Path $b|Out-Null
 Copy-Item -LiteralPath $target -Destination $b -Recurse
 Copy-Item -LiteralPath $statePath -Destination (Join-Path $b 'install-state.json')
 $bh=TreeHash (Join-Path $b 'SkellFan.vst3');$sh=Hash (Join-Path $b 'install-state.json')
 if($bh -ne $s.bundle_sha256){throw 'Backup bundle hash mismatch'}
 SaveJson (Join-Path $b 'backup-manifest.json') ([ordered]@{product='SkellFan';version=$s.version;bundle_sha256=$bh;state_sha256=$sh;created_utc=[DateTime]::UtcNow.ToString('o')})
 $b
}
function VerifyBackup([string]$b,[string]$backupRoot) {
 $full=SafePath $b;$prefix=(Full $backupRoot).TrimEnd([char[]]@('\','/'))+[IO.Path]::DirectorySeparatorChar
 if(!$full.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Rollback source must be inside SkellFan Backups'}
 NoLinks $full
 $s=Json (Join-Path $full 'install-state.json');$m=Json (Join-Path $full 'backup-manifest.json');$bundle=Join-Path $full 'SkellFan.vst3'
 if($m.product -ne 'SkellFan'-or $m.version -ne $s.version-or (TreeHash $bundle) -ne $m.bundle_sha256-or $m.bundle_sha256 -ne $s.bundle_sha256-or (Hash (Join-Path $full 'install-state.json')) -ne $m.state_sha256){throw 'Rollback backup checksum verification failed'}
 ValidateBundle $bundle $s.version|Out-Null
 [pscustomobject]@{Path=$full;State=$s;Bundle=$bundle;Hash=$m.bundle_sha256}
}
function InstallPreview {
 $pkg=ValidatePackage;$install=SafePath $InstallRoot;$data=SafePath $DataRoot
 if($install -eq $data-or $install.StartsWith($data+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)-or $data.StartsWith($install+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'InstallRoot and DataRoot must be separate'}
 $backupRoot=Join-Path $data 'Backups';[void](SafePath $backupRoot)
 $target=Join-Path $install 'SkellFan.vst3';$statePath=Join-Path $data 'install-state.json';$old=State $target $statePath
 if($old-and $old.version -eq $pkg.Manifest.version-and $old.bundle_sha256 -eq $pkg.BundleHash){return [pscustomobject]@{Status='AlreadyCurrent';BackupPath=$null;Version=$old.version}}
 if(!$Approve-and !$WhatIfPreference){throw 'Review -WhatIf, then pass -Approve to install/update'}
 if(!$PSCmdlet.ShouldProcess($target,"Install SkellFan preview $($pkg.Manifest.version)")){return [pscustomobject]@{Status='WhatIf';BackupPath=$null;Version=$pkg.Manifest.version}}
 New-Item -ItemType Directory -Path $install,$data -Force|Out-Null
 $backup=$null;if($old){$backup=Backup $target $statePath $backupRoot $old}
 $token=[guid]::NewGuid().ToString('N');$stage=Join-Path $install ('.SkellFan-stage-'+$token);$oldPath=Join-Path $install ('.SkellFan-old-'+$token)
 $oldMoved=$false;$newMoved=$false
 try{
  New-Item -ItemType Directory -Path $stage|Out-Null;Copy-Item -LiteralPath $pkg.Bundle -Destination $stage -Recurse
  $staged=Join-Path $stage 'SkellFan.vst3';if((TreeHash $staged)-ne $pkg.BundleHash){throw 'Staged bundle hash mismatch'}
  if($old){Move-Item -LiteralPath $target -Destination $oldPath;$oldMoved=$true}
  Move-Item -LiteralPath $staged -Destination $target;$newMoved=$true
  if((TreeHash $target)-ne $pkg.BundleHash){throw 'Installed bundle hash mismatch'}
  SaveJson $statePath ([ordered]@{product='SkellFan';version=$pkg.Manifest.version;install_path=$target;bundle_sha256=$pkg.BundleHash;source_commit=$pkg.Manifest.source_commit;workflow_run_id=$pkg.Manifest.workflow_run_id;installed_utc=[DateTime]::UtcNow.ToString('o')})
  if($oldMoved-and(Test-Path $oldPath)){Remove-Item -LiteralPath $oldPath -Recurse -Force -ErrorAction SilentlyContinue}
  [pscustomobject]@{Status=$(if($old){'Updated'}else{'Installed'});BackupPath=$backup;Version=$pkg.Manifest.version}
 }catch{
  if($newMoved-and(Test-Path $target)){Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction SilentlyContinue}
  if($oldMoved-and(Test-Path $oldPath)){Move-Item -LiteralPath $oldPath -Destination $target -Force}
  if($old-and $backup){Copy-Item -LiteralPath (Join-Path $backup 'install-state.json') -Destination $statePath -Force}elseif(Test-Path $statePath){Remove-Item -LiteralPath $statePath -Force}
  throw
 }finally{if(Test-Path $stage){Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue}}
}
function RollbackPreview {
 $install=SafePath $InstallRoot;$data=SafePath $DataRoot;$backupRoot=Join-Path $data 'Backups';[void](SafePath $backupRoot)
 $backup=VerifyBackup $RollbackBackup $backupRoot;$target=Join-Path $install 'SkellFan.vst3';$statePath=Join-Path $data 'install-state.json';$old=State $target $statePath
 if(!$Approve-and !$WhatIfPreference){throw 'Review -WhatIf, then pass -Approve to roll back'}
 if(!$PSCmdlet.ShouldProcess($target,"Restore SkellFan $($backup.State.version)")){return [pscustomobject]@{Status='WhatIf';Version=$backup.State.version}}
 New-Item -ItemType Directory -Path $install,$data -Force|Out-Null
 $current=$null;if($old){$current=Backup $target $statePath $backupRoot $old}
 $token=[guid]::NewGuid().ToString('N');$stage=Join-Path $install ('.SkellFan-rollback-stage-'+$token);$oldPath=Join-Path $install ('.SkellFan-rollback-old-'+$token)
 $oldMoved=$false;$newMoved=$false
 try{
  New-Item -ItemType Directory -Path $stage|Out-Null;Copy-Item -LiteralPath $backup.Bundle -Destination $stage -Recurse
  $staged=Join-Path $stage 'SkellFan.vst3';if((TreeHash $staged)-ne $backup.Hash){throw 'Rollback staging checksum mismatch'}
  if($old){Move-Item -LiteralPath $target -Destination $oldPath;$oldMoved=$true}
  Move-Item -LiteralPath $staged -Destination $target;$newMoved=$true
  if((TreeHash $target)-ne $backup.Hash){throw 'Restored bundle checksum mismatch'}
  $s=$backup.State;$s.install_path=$target;$s.bundle_sha256=$backup.Hash;$s.restored_from_backup=$backup.Path;$s.installed_utc=[DateTime]::UtcNow.ToString('o');SaveJson $statePath $s
  if($oldMoved-and(Test-Path $oldPath)){Remove-Item -LiteralPath $oldPath -Recurse -Force -ErrorAction SilentlyContinue}
  [pscustomobject]@{Status='RolledBack';Version=$s.version;BackupPath=$current}
 }catch{
  if($newMoved-and(Test-Path $target)){Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction SilentlyContinue}
  if($oldMoved-and(Test-Path $oldPath)){Move-Item -LiteralPath $oldPath -Destination $target -Force}
  if($old-and $current){Copy-Item -LiteralPath (Join-Path $current 'install-state.json') -Destination $statePath -Force}elseif(Test-Path $statePath){Remove-Item -LiteralPath $statePath -Force}
  throw
 }finally{if(Test-Path $stage){Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue}}
}
if(!$env:LOCALAPPDATA){throw 'Windows LOCALAPPDATA is required'}
if($RollbackBackup){RollbackPreview}else{InstallPreview}
