# Generate TypeScript for all TestObject and MyObject classes
$ErrorActionPreference = "SilentlyContinue"

# Activate conda environment
conda activate webbridge_hackathon

# Generate TestObject base class
Write-Host "Generating TestObject..."
python tools/generate.py src/TestObject.h --class-name TestObject  --ts_impl_out frontend/src

Write-Host "Generating MyObject..."
python tools/generate.py src/MyObject.h --class-name MyObject  --ts_impl_out frontend/src

# Generate TestObject1 through TestObject35
for ($i = 1; $i -le 35; $i++) {
    $className = "TestObject$i"
    $filePath = "src/$className.h"
    
    if (Test-Path $filePath) {
        Write-Host "Generating $className..."
        python tools/generate.py $filePath --class-name $className  --ts_impl_out frontend/src
    }
}

Write-Host "Done!"
