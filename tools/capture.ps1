# Dev tool: start the game with some arguments, screenshot its window after each delay, close it.
# Works even when the window is covered (PrintWindow), and needs no mouse/keyboard.
#
#   powershell -ExecutionPolicy Bypass -File tools/capture.ps1 -Exe build/debug/NinjaTurtlesGame.exe `
#       -GameArgs "--gallery turtles" -Delays "2" -Out shots/turtles
#   -> shots/turtles_0.png
#
#   ... -GameArgs ""        -> main menu
#   ... -GameArgs "--demo" -Delays "10,20,30" -Out shots/demo   -> shots/demo_0.png, _1.png, _2.png
# (plain strings so it also works when called from bash / cmd)
param(
    [string]$Exe = "build/debug/NinjaTurtlesGame.exe",
    [string]$GameArgs = "--gallery turtles",
    [string]$Delays = "2",
    [string]$Out = "capture"
)
$delayList = @($Delays -split "," | ForEach-Object { [double]$_.Trim() })

$root = Split-Path -Parent $PSScriptRoot
if (-not [System.IO.Path]::IsPathRooted($Exe)) { $Exe = Join-Path $root $Exe }
if (-not [System.IO.Path]::IsPathRooted($Out)) { $Out = Join-Path (Get-Location) $Out }
$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir | Out-Null }

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System; using System.Runtime.InteropServices;
public class CaptureWin {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    public struct RECT { public int L, T, R, B; }
}
"@
[CaptureWin]::SetProcessDPIAware() | Out-Null

# Start-Process rejects an empty argument list, so leave it out for a plain run (main menu)
if ($GameArgs.Trim()) { $p = Start-Process -FilePath $Exe -ArgumentList $GameArgs -WorkingDirectory $root -PassThru }
else                  { $p = Start-Process -FilePath $Exe -WorkingDirectory $root -PassThru }
try {
    $waited = 0
    while ($p.MainWindowHandle -eq 0 -and $waited -lt 100) { Start-Sleep -Milliseconds 100; $p.Refresh(); $waited++ }
    if ($p.MainWindowHandle -eq 0) { throw "game window never appeared (did it crash? run the exe by hand to see the console)" }
    $h = $p.MainWindowHandle

    $elapsed = 0.0
    for ($i = 0; $i -lt $delayList.Count; $i++) {
        $wait = $delayList[$i] - $elapsed
        if ($wait -gt 0) { Start-Sleep -Milliseconds ([int]($wait * 1000)) }
        $elapsed = $delayList[$i]
        if ($p.HasExited) { throw "game exited early with code $($p.ExitCode)" }

        $r = New-Object CaptureWin+RECT
        [CaptureWin]::GetClientRect($h, [ref]$r) | Out-Null
        $bmp = New-Object System.Drawing.Bitmap ([Math]::Max(1, $r.R)), ([Math]::Max(1, $r.B))
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $hdc = $g.GetHdc()
        [CaptureWin]::PrintWindow($h, $hdc, 3) | Out-Null   # PW_CLIENTONLY | PW_RENDERFULLCONTENT
        $g.ReleaseHdc($hdc); $g.Dispose()
        $file = "${Out}_$i.png"
        $bmp.Save($file, [System.Drawing.Imaging.ImageFormat]::Png); $bmp.Dispose()
        Write-Output "saved $file"
    }
}
finally {
    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
}
