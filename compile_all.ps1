# Compile all .ino sketches in SmartFall and its Tests subdirectories using arduino-cli
# Logs output to compile_all.log and runs test compilations concurrently using PowerShell jobs


$ErrorActionPreference = 'Stop'

# Default board FQBN (can be overridden by argument)
$DEFAULT_FQBN = "esp32:esp32:adafruit_feather_esp32_v2"
$FQBN = if ($args.Count -ge 1) { $args[0] } else { $DEFAULT_FQBN }

Write-Host "Using board FQBN: $FQBN"

$ROOT_DIR = Join-Path $PSScriptRoot 'SmartFall'

$LOG_FILE = Join-Path $PSScriptRoot 'compile_all.log'
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

# Check if compile_all.log is accessible (not locked by another process)
$logFileAccessible = $true
try {
    $logStream = [System.IO.File]::Open($LOG_FILE, [System.IO.FileMode]::OpenOrCreate, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    $logStream.Close()
} catch {
    Write-Host "Error: compile_all.log is in use by another process. Please close any program using it and try again."
    exit 1
}

"\n==============================" | Out-File -FilePath $LOG_FILE
"Compilation log started at $timestamp" | Out-File -FilePath $LOG_FILE -Append
"==============================\n" | Out-File -FilePath $LOG_FILE -Append

# Check if arduino-cli is available
if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    Write-Host "Error: arduino-cli not found in PATH. Please install arduino-cli and ensure it is available in your PATH."
    "Error: arduino-cli not found in PATH." | Out-File -FilePath $LOG_FILE -Append
    exit 1
}

$anyFail = $false

# Compile main sketch
$MAIN_SKETCH = Join-Path $ROOT_DIR 'SmartFall.ino'
if (Test-Path $MAIN_SKETCH) {
    Write-Host "Compiling main SmartFall.ino..."
    "Compiling main SmartFall.ino..." | Out-File -FilePath $LOG_FILE -Append
    try {
        arduino-cli compile --fqbn $FQBN $ROOT_DIR *>> $LOG_FILE
        "Main SmartFall.ino compiled successfully." | Out-File -FilePath $LOG_FILE -Append
    } catch {
        "Main SmartFall.ino compilation failed." | Out-File -FilePath $LOG_FILE -Append
        $anyFail = $true
    }
} else {
    "Main SmartFall.ino not found." | Out-File -FilePath $LOG_FILE -Append
    $anyFail = $true
}

# Compile test sketches concurrently
$TESTS_DIR = Join-Path $ROOT_DIR 'Tests'
$jobs = @()
$tempLogDir = Join-Path $PSScriptRoot 'temp_logs'
if (Test-Path $tempLogDir) { Remove-Item $tempLogDir -Recurse -Force }
New-Item -ItemType Directory -Path $tempLogDir | Out-Null

if (Test-Path $TESTS_DIR) {
    Get-ChildItem -Directory $TESTS_DIR | ForEach-Object {
        $TEST_SUBDIR = $_.FullName
        Get-ChildItem -Path $TEST_SUBDIR -Filter *.ino | ForEach-Object {
            $SKETCH = $_.FullName
            $SKETCH_NAME = $_.Name
            $SUBDIR_NAME = (Split-Path $TEST_SUBDIR -Leaf)
            Write-Host "Compiling $SKETCH_NAME in $SUBDIR_NAME..."
            "Compiling $SKETCH_NAME in $SUBDIR_NAME..." | Out-File -FilePath $LOG_FILE -Append
            
            $tempLogFile = Join-Path $tempLogDir "$SUBDIR_NAME-$([System.IO.Path]::GetRandomFileName()).log"
            $jobs += Start-Job -ScriptBlock {
                param($FQBN, $TEST_SUBDIR, $SKETCH_NAME, $SUBDIR_NAME, $tempLogFile)
                $result = $true
                try {
                    arduino-cli compile --fqbn $FQBN $TEST_SUBDIR *>> $tempLogFile
                    "$SKETCH_NAME in $SUBDIR_NAME compiled successfully." | Out-File -FilePath $tempLogFile -Append
                } catch {
                    "$SKETCH_NAME in $SUBDIR_NAME compilation failed." | Out-File -FilePath $tempLogFile -Append
                    $result = $false
                }
                return $result
            } -ArgumentList $FQBN, $TEST_SUBDIR, $SKETCH_NAME, $SUBDIR_NAME, $tempLogFile
        }
    }
    # Wait for all jobs to finish and check results
    foreach ($job in $jobs) {
        Wait-Job $job
        $output = Receive-Job $job
        if ($output -eq $false) {
            $anyFail = $true
        }
        Remove-Job $job
    }
    
    # Consolidate temp logs into main log file
    Get-ChildItem -Path $tempLogDir -Filter *.log | ForEach-Object {
        Get-Content $_.FullName | Out-File -FilePath $LOG_FILE -Append
    }
    
    # Clean up temp log directory
    Remove-Item $tempLogDir -Recurse -Force
} else {
    "Tests directory not found." | Out-File -FilePath $LOG_FILE -Append
    $anyFail = $true
}

if ($anyFail) {
    Write-Host "One or more compilations failed. See $LOG_FILE for details."
    exit 1
} else {
    Write-Host "All compilations succeeded. See $LOG_FILE for details."
    exit 0
}
