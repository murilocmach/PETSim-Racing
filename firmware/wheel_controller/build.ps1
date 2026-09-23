#requires -Version 5
$ErrorActionPreference = "Stop"

$Toolchain = "C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin"
$CC = Join-Path $Toolchain "arm-none-eabi-gcc.exe"
$OBJCOPY = Join-Path $Toolchain "arm-none-eabi-objcopy.exe"
$SIZE = Join-Path $Toolchain "arm-none-eabi-size.exe"

$Target = "wheel_controller"
$BuildDir = "build"

$CSources = @(
  "Src\main.c",
  "Src\stm32f4xx_it.c",
  "Src\stm32f4xx_hal_msp.c",
  "Src\syscalls.c",
  "Src\sysmem.c",
  "Drivers\CMSIS\Device\ST\STM32F4xx\Source\Templates\system_stm32f4xx.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_rcc.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_rcc_ex.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_gpio.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_uart.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_cortex.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_pwr.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_pwr_ex.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_flash.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_flash_ex.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_flash_ramfunc.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_dma.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_dma_ex.c",
  "Drivers\STM32F4xx_HAL_Driver\Src\stm32f4xx_hal_exti.c"
)
$AsmSources = @("Startup\startup_stm32f411ceux.s")

$Includes = @(
  "-IInc",
  "-IDrivers\STM32F4xx_HAL_Driver\Inc",
  "-IDrivers\CMSIS\Device\ST\STM32F4xx\Include",
  "-IDrivers\CMSIS\Include"
)

$Mcu = @("-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard")
$Defs = @("-DSTM32F411xE")
$CFlags = $Mcu + $Defs + $Includes + @("-Wall", "-Og", "-g3", "-std=gnu11", "-ffunction-sections", "-fdata-sections", "-MMD", "-MP")
$AsFlags = $Mcu + @("-g3", "-x", "assembler-with-cpp")

if (-not (Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }

$Objects = @()

foreach ($src in $CSources) {
  $obj = Join-Path $BuildDir ((Split-Path $src -Leaf) -replace '\.c$', '.o')
  Write-Host "CC  $src"
  & $CC -c @CFlags $src -o $obj
  if ($LASTEXITCODE -ne 0) { throw "Failed compiling $src" }
  $Objects += $obj
}

foreach ($src in $AsmSources) {
  $obj = Join-Path $BuildDir ((Split-Path $src -Leaf) -replace '\.s$', '.o')
  Write-Host "AS  $src"
  & $CC -c @AsFlags $src -o $obj
  if ($LASTEXITCODE -ne 0) { throw "Failed assembling $src" }
  $Objects += $obj
}

$Elf = Join-Path $BuildDir "$Target.elf"
$Map = Join-Path $BuildDir "$Target.map"
$LdFlags = $Mcu + @("-specs=nano.specs", "-TSTM32F411CEUX_FLASH.ld", "-Wl,--gc-sections", "-Wl,-Map=$Map", "-lc", "-lm", "-lnosys")

Write-Host "LD  $Elf"
& $CC @Objects @LdFlags -o $Elf
if ($LASTEXITCODE -ne 0) { throw "Link failed" }

& $OBJCOPY -O ihex $Elf (Join-Path $BuildDir "$Target.hex")
& $OBJCOPY -O binary -S $Elf (Join-Path $BuildDir "$Target.bin")

& $SIZE $Elf

Write-Host "`nBuild OK -> $Elf"
