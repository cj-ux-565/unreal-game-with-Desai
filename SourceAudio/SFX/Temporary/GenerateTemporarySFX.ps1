param(
    [string]$OutputDirectory = $PSScriptRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

$source = @'
using System;
using System.IO;

public static class SOTMTemporarySfxGenerator
{
    const int SampleRate = 48000;

    static double Envelope(double t, double duration, double attack, double release)
    {
        double a = Math.Min(1.0, t / Math.Max(0.001, attack));
        double r = Math.Min(1.0, (duration - t) / Math.Max(0.001, release));
        return Math.Max(0.0, Math.Min(a, r));
    }

    static void WriteWave(string path, double duration, Func<double, Random, double> sample)
    {
        int frames = (int)(duration * SampleRate);
        int dataSize = frames * 2;
        using (var writer = new BinaryWriter(File.Create(path)))
        {
            writer.Write(new char[] {'R','I','F','F'});
            writer.Write(36 + dataSize);
            writer.Write(new char[] {'W','A','V','E'});
            writer.Write(new char[] {'f','m','t',' '});
            writer.Write(16);
            writer.Write((short)1);
            writer.Write((short)1);
            writer.Write(SampleRate);
            writer.Write(SampleRate * 2);
            writer.Write((short)2);
            writer.Write((short)16);
            writer.Write(new char[] {'d','a','t','a'});
            writer.Write(dataSize);
            var random = new Random(1987);
            for (int i = 0; i < frames; ++i)
            {
                double t = (double)i / SampleRate;
                double value = Math.Max(-1.0, Math.Min(1.0, sample(t, random)));
                writer.Write((short)(value * 32767.0));
            }
        }
    }

    public static void Generate(string directory)
    {
        WriteWave(Path.Combine(directory, "SFX_TEMP_KnockoutImpact.wav"), 1.15, (t, r) =>
            Envelope(t, 1.15, 0.006, 0.9) * (0.58 * Math.Sin(2 * Math.PI * (78 - 34 * t) * t) +
            0.20 * (r.NextDouble() * 2 - 1) * Math.Exp(-7 * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_Respawn.wav"), 1.10, (t, r) =>
            Envelope(t, 1.10, 0.03, 0.28) * 0.34 * (Math.Sin(2 * Math.PI * (220 + 250 * t) * t) +
            0.45 * Math.Sin(2 * Math.PI * (330 + 360 * t) * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_StationOpen.wav"), 0.42, (t, r) =>
            Envelope(t, 0.42, 0.008, 0.25) * 0.30 * (Math.Sin(2 * Math.PI * 310 * t) +
            0.7 * Math.Sin(2 * Math.PI * 465 * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_Denied.wav"), 0.38, (t, r) =>
            Envelope(t, 0.38, 0.005, 0.23) * 0.40 * Math.Sin(2 * Math.PI * (145 - 30 * t) * t));

        WriteWave(Path.Combine(directory, "SFX_TEMP_UpgradeSuccess.wav"), 1.20, (t, r) =>
            Envelope(t, 1.20, 0.006, 0.52) * 0.24 * (Math.Sin(2 * Math.PI * 293.66 * t) +
            0.7 * Math.Sin(2 * Math.PI * 440.0 * t) + 0.42 * Math.Sin(2 * Math.PI * 587.33 * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_ChestOpen.wav"), 1.25, (t, r) =>
            Envelope(t, 1.25, 0.008, 0.50) * (0.18 * (r.NextDouble() * 2 - 1) * Math.Exp(-1.8 * t) +
            0.23 * Math.Sin(2 * Math.PI * (92 - 28 * t) * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_KeyAcquired.wav"), 0.90, (t, r) =>
            Envelope(t, 0.90, 0.004, 0.48) * 0.25 * (Math.Sin(2 * Math.PI * 523.25 * t) +
            0.62 * Math.Sin(2 * Math.PI * 783.99 * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_GateLocked.wav"), 0.62, (t, r) =>
            Envelope(t, 0.62, 0.004, 0.32) * (0.34 * Math.Sin(2 * Math.PI * 88 * t) +
            0.12 * (r.NextDouble() * 2 - 1) * Math.Exp(-6 * t)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_GateOpen.wav"), 3.10, (t, r) =>
            Envelope(t, 3.10, 0.12, 0.70) * (0.22 * Math.Sin(2 * Math.PI * (52 + 4 * Math.Sin(t)) * t) +
            0.10 * (r.NextDouble() * 2 - 1)));

        WriteWave(Path.Combine(directory, "SFX_TEMP_DemoComplete.wav"), 2.40, (t, r) =>
            Envelope(t, 2.40, 0.02, 0.85) * 0.20 * (Math.Sin(2 * Math.PI * 146.83 * t) +
            0.66 * Math.Sin(2 * Math.PI * 220.0 * t) + 0.40 * Math.Sin(2 * Math.PI * 293.66 * t)));
    }
}
'@

Add-Type -TypeDefinition $source -Language CSharp
[SOTMTemporarySfxGenerator]::Generate($OutputDirectory)

Get-ChildItem -LiteralPath $OutputDirectory -Filter 'SFX_TEMP_*.wav' |
    Sort-Object Name |
    ForEach-Object {
        [PSCustomObject]@{
            File = $_.Name
            Source = 'Procedurally generated in-project; no third-party recording or download'
            License = 'Project-owned temporary placeholder SFX'
            SampleRate = 48000
            Bits = 16
            Channels = 1
            Sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
        }
    } | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'TemporarySFXManifest.json') -Encoding utf8

Write-Output 'Generated 10 project-owned TEMPORARY PLACEHOLDER SFX files.'

