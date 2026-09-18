param(
    [string]$OutputDirectory = $PSScriptRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Speech

$audioFormat = New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(
    48000,
    [System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,
    [System.Speech.AudioFormat.AudioChannel]::Mono
)

$lines = @(
    @{ File = 'VO_TEMP_Timmy_Mansion_001.wav'; Voice = 'Microsoft David Desktop'; Rate = 0; Volume = 92; Text = "Hi I’m Timmy Bottom smith it’s nice to meet you mage I’m so glad you came. Listen if you want your powers back you need to go that forest and get it. She’s been waiting for you. Isabella. She took everything from you… even your powers and there’s only one way back good luck." },
    @{ File = 'VO_TEMP_Timmy_Mansion_002.wav'; Voice = 'Microsoft David Desktop'; Rate = -1; Volume = 92; Text = "But listen… you can get them back. Just not in here." },
    @{ File = 'VO_TEMP_Timmy_Mansion_003.wav'; Voice = 'Microsoft David Desktop'; Rate = -2; Volume = 86; Text = "Oh no… she’s coming—" },
    @{ File = 'VO_TEMP_Isabella_Mansion_001.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -2; Volume = 92; Text = "I knew you’d come back…" },
    @{ File = 'VO_TEMP_Isabella_Mansion_002.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -2; Volume = 92; Text = "You always do." },
    @{ File = 'VO_TEMP_Isabella_Mansion_003.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -2; Volume = 94; Text = "No speed. No lightning. You’re mine again." },
    @{ File = 'VO_TEMP_Isabella_Mansion_004.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -3; Volume = 88; Text = "If you want to run so bad… let’s see how far you get." },
    @{ File = 'VO_TEMP_Timmy_Forest_001.wav'; Voice = 'Microsoft David Desktop'; Rate = -1; Volume = 92; Text = "You’re in her Forest Domain… the only place you can regain your powers." },
    @{ File = 'VO_TEMP_Timmy_Forest_002.wav'; Voice = 'Microsoft David Desktop'; Rate = 0; Volume = 92; Text = "Collect those. They hold pieces of your speed and lightning." },
    @{ File = 'VO_TEMP_Timmy_Upgrade_001.wav'; Voice = 'Microsoft David Desktop'; Rate = 0; Volume = 92; Text = "Okay… I can restore your speed boost. But use it wisely. There’s a cooldown." },
    @{ File = 'VO_TEMP_Timmy_Upgrade_002.wav'; Voice = 'Microsoft David Desktop'; Rate = -1; Volume = 92; Text = "Run when they’re close. Or you die." },
    @{ File = 'VO_TEMP_Timmy_Chest_001.wav'; Voice = 'Microsoft David Desktop'; Rate = -1; Volume = 92; Text = "That key leads to her true form. You’ll need both abilities to survive." },
    @{ File = 'VO_TEMP_Cousin_Whisper_001.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -3; Volume = 74; Text = "He’s back…" },
    @{ File = 'VO_TEMP_Cousin_Whisper_002.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -3; Volume = 74; Text = "He can’t escape…" },
    @{ File = 'VO_TEMP_Cousin_Whisper_003.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -3; Volume = 74; Text = "Sister wants him…" },
    @{ File = 'VO_TEMP_Cousin_Whisper_004.wav'; Voice = 'Microsoft Zira Desktop'; Rate = -2; Volume = 74; Text = "Let’s play chase…" },
    @{ File = 'VO_TEMP_Cousin_Whisper_005.wav'; Voice = 'Microsoft Zira Desktop'; Rate = 1; Volume = 80; Text = "RUN RUN RUN!" },
    @{ File = 'VO_TEMP_Cousin_Detect_001.wav'; Voice = 'Microsoft Zira Desktop'; Rate = 1; Volume = 96; Text = "FOUND YOU!" },
    @{ File = 'VO_TEMP_Cousin_Detect_002.wav'; Voice = 'Microsoft Zira Desktop'; Rate = 0; Volume = 96; Text = "FRESH MEAT!" },
    @{ File = 'VO_TEMP_Cousin_Detect_003.wav'; Voice = 'Microsoft Zira Desktop'; Rate = 0; Volume = 96; Text = "SISTER!" },
    @{ File = 'VO_TEMP_Cousin_Detect_004.wav'; Voice = 'Microsoft Zira Desktop'; Rate = 1; Volume = 96; Text = "HE’S HERE!" }
)

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$manifest = @()

foreach ($line in $lines) {
    $outputPath = Join-Path $OutputDirectory $line.File
    $synth = New-Object System.Speech.Synthesis.SpeechSynthesizer
    try {
        $synth.SelectVoice($line.Voice)
        $synth.Rate = $line.Rate
        $synth.Volume = $line.Volume
        $synth.SetOutputToWaveFile($outputPath, $audioFormat)
        $synth.Speak($line.Text)
    }
    finally {
        $synth.Dispose()
    }

    $manifest += [PSCustomObject]@{
        File = $line.File
        Speaker = if ($line.File -like '*Timmy*') { 'Timmy' } elseif ($line.File -like '*Isabella*') { 'Isabella' } else { 'Cousin' }
        Voice = $line.Voice
        Rate = $line.Rate
        Volume = $line.Volume
        SampleRate = 48000
        Bits = 16
        Channels = 1
        Text = $line.Text
        Sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $outputPath).Hash
    }
}

$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'TemporaryDialogueManifest.json') -Encoding utf8
Write-Output "Generated $($lines.Count) TEMPORARY PLACEHOLDER VO files in $OutputDirectory"
