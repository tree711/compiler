$ErrorActionPreference = "Continue"

$compiler = Join-Path $PSScriptRoot "compiler-tests.exe"
$total = 0
$passed = 0
$failed = 0

function Record-Result {
    param([string]$Name, [bool]$Success)

    $script:total++
    if ($Success) {
        $script:passed++
        Write-Host "[PASS] $Name"
    } else {
        $script:failed++
        Write-Host "[FAIL] $Name"
    }
}

function Invoke-Native {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments *> $null
    return $LASTEXITCODE
}

$sources = @(
    "src/main.cpp",
    "src/Lexer.cpp",
    "src/Parser.cpp",
    "src/Semantic.cpp",
    "src/IRGenerator.cpp",
    "src/Optimizer.cpp",
    "src/CodeGenerator.cpp"
)

$buildArgs = @("-std=c++17", "-Wall", "-Wextra", "-Iinclude") + $sources + @("-o", $compiler)
if ((Invoke-Native "g++" $buildArgs) -ne 0) {
    throw "Compiler build failed."
}

$validTests = @(
    "test.c",
    "test1_arithmetic.c",
    "test2_nested_if.c",
    "test3_nested_while.c",
    "test4_complex_expr.c",
    "test5_comparison.c",
    "test6_edge_cases.c"
)

foreach ($test in $validTests) {
    foreach ($optimized in @($false, $true)) {
        $mode = if ($optimized) { "optimized" } else { "normal" }
        $stem = [IO.Path]::GetFileNameWithoutExtension($test)
        $assembly = Join-Path $PSScriptRoot "${stem}_${mode}.s"
        $executable = Join-Path $PSScriptRoot "${stem}_${mode}.exe"
        $compilerArgs = @($test, "-o", $assembly)
        if ($optimized) {
            $compilerArgs += "-O"
        }

        if ((Invoke-Native $compiler $compilerArgs) -ne 0) {
            Record-Result "$test ($mode): compile" $false
            continue
        }
        if ((Invoke-Native "g++" @($assembly, "-o", $executable)) -ne 0) {
            Record-Result "$test ($mode): assemble/link" $false
            continue
        }

        $process = Start-Process -FilePath $executable -PassThru -WindowStyle Hidden
        if (-not $process.WaitForExit(3000)) {
            $process.Kill()
            $process.WaitForExit()
            Record-Result "$test ($mode): timeout" $false
            continue
        }

        Record-Result "$test ($mode)" ($process.ExitCode -eq 0)
    }
}

$invalidTests = @(
    "test_invalid_syntax.c",
    "test_invalid_undefined.c",
    "test_invalid_redeclaration.c",
    "test_invalid_divzero.c"
)

foreach ($test in $invalidTests) {
    $assembly = Join-Path $PSScriptRoot "$([IO.Path]::GetFileNameWithoutExtension($test))_invalid.s"
    $exitCode = Invoke-Native $compiler @($test, "-o", $assembly)
    Record-Result "$test (expected rejection)" ($exitCode -ne 0)
}

Write-Host ""
Write-Host "Total: $total  Passed: $passed  Failed: $failed"
exit $failed
